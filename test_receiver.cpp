#include "libwarpdeck/include/warpdeck.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <filesystem>

bool transfer_received = false;
bool peer_found = false;
std::string pending_transfer_id;

void peer_discovered_callback(const char* peer_json) {
    std::cout << "🎯 Peer discovered: " << peer_json << std::endl;
    
    // Check if it's a sender device
    std::string json_str(peer_json);
    if (json_str.find("Sender-Device") != std::string::npos || 
        json_str.find("sender-001") != std::string::npos) {
        peer_found = true;
        std::cout << "📡 Found sender device!" << std::endl;
    }
}

void peer_lost_callback(const char* device_id) {
    std::cout << "📤 Peer lost: " << device_id << std::endl;
}

void transfer_request_callback(const char* request_json) {
    std::cout << "📨 Incoming transfer request: " << request_json << std::endl;
    
    // Extract transfer ID (simplified parsing)
    std::string json_str(request_json);
    size_t id_pos = json_str.find("\"transfer_id\":");
    if (id_pos != std::string::npos) {
        size_t start = json_str.find("\"", id_pos + 14) + 1;
        size_t end = json_str.find("\"", start);
        pending_transfer_id = json_str.substr(start, end - start);
        std::cout << "📥 Transfer ID: " << pending_transfer_id << std::endl;
    }
    
    transfer_received = true;
}

void transfer_progress_callback(const char* transfer_id, float progress_percent, uint64_t bytes_transferred) {
    std::cout << "📊 Transfer progress - ID: " << transfer_id << ", Progress: " << progress_percent << "%, Bytes: " << bytes_transferred << std::endl;
}

void transfer_completed_callback(const char* transfer_id, bool success, const char* error_message) {
    std::cout << "✅ Transfer completed - ID: " << transfer_id << ", Success: " << (success ? "YES" : "NO");
    if (error_message) {
        std::cout << ", Error: " << error_message;
    }
    std::cout << std::endl;
    
    if (success) {
        std::cout << "🎉 Files received successfully!" << std::endl;
        std::cout << "📁 Checking received files:" << std::endl;
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator("/app/received_files")) {
                if (entry.is_regular_file()) {
                    std::cout << "  - " << entry.path().filename().string() 
                              << " (" << std::filesystem::file_size(entry) << " bytes)" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "  (Error listing received files: " << e.what() << ")" << std::endl;
        }
    }
}

void error_callback(const char* error_message) {
    std::cout << "❌ Error: " << error_message << std::endl;
}

int main() {
    std::cout << "📡 Starting WarpDeck Receiver..." << std::endl;
    
    // Initialize WarpDeck callbacks
    Callbacks callbacks = {0};
    callbacks.on_peer_discovered = peer_discovered_callback;
    callbacks.on_peer_lost = peer_lost_callback;
    callbacks.on_incoming_transfer_request = transfer_request_callback;
    callbacks.on_transfer_progress_update = transfer_progress_callback;
    callbacks.on_transfer_completed = transfer_completed_callback;
    callbacks.on_error = error_callback;
    
    WarpDeckHandle* handle = warpdeck_create(&callbacks, "/tmp/warpdeck_receiver");
    if (!handle) {
        std::cerr << "❌ Failed to create WarpDeck handle" << std::endl;
        return 1;
    }
    
    // Start WarpDeck
    const char* device_name = "Receiver-Device";
    
    if (!warpdeck_start(handle, device_name, 0)) {
        std::cerr << "❌ Failed to start WarpDeck" << std::endl;
        warpdeck_destroy(handle);
        return 1;
    }
    
    std::cout << "✅ WarpDeck started successfully!" << std::endl;
    std::cout << "📱 Device: " << device_name << std::endl;
    std::cout << "🔍 Waiting for sender device and transfer requests..." << std::endl;
    
    // Wait for peer discovery and transfer request
    int total_wait_time = 120; // 2 minutes
    for (int i = 0; i < total_wait_time; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Auto-accept any transfer requests
        if (transfer_received && !pending_transfer_id.empty()) {
            std::cout << "🤝 Auto-accepting transfer request: " << pending_transfer_id << std::endl;
            warpdeck_respond_to_transfer(handle, pending_transfer_id.c_str(), true);
            pending_transfer_id.clear();
            transfer_received = false;
        }
        
        // Periodic status updates
        if (i % 15 == 0) {
            std::cout << "⏳ Listening... (" << i << "/" << total_wait_time << "s)";
            if (peer_found) {
                std::cout << " - Sender device connected!";
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "📁 Final check of received files:" << std::endl;
    try {
        bool has_files = false;
        for (const auto& entry : std::filesystem::directory_iterator("/app/received_files")) {
            if (entry.is_regular_file()) {
                std::cout << "  ✅ " << entry.path().filename().string() 
                          << " (" << std::filesystem::file_size(entry) << " bytes)" << std::endl;
                has_files = true;
            }
        }
        if (!has_files) {
            std::cout << "  (No files received)" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "  (Error checking files: " << e.what() << ")" << std::endl;
    }
    
    // Cleanup
    warpdeck_stop(handle);
    warpdeck_destroy(handle);
    
    std::cout << "🏁 Receiver test completed." << std::endl;
    return 0;
}