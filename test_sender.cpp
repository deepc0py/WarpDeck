#include "libwarpdeck/include/warpdeck.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <filesystem>

std::string discovered_receiver_id;
bool transfer_completed = false;
bool peer_found = false;

void peer_discovered_callback(const char* peer_json) {
    std::cout << "🎯 Peer discovered: " << peer_json << std::endl;
    
    // Parse to find receiver
    std::string json_str(peer_json);
    if (json_str.find("Receiver-Device") != std::string::npos || 
        json_str.find("receiver-001") != std::string::npos) {
        
        // Extract device ID (simplified parsing)
        size_t id_pos = json_str.find("\"id\":");
        if (id_pos != std::string::npos) {
            size_t start = json_str.find("\"", id_pos + 5) + 1;
            size_t end = json_str.find("\"", start);
            discovered_receiver_id = json_str.substr(start, end - start);
            peer_found = true;
            std::cout << "📡 Found receiver device with ID: " << discovered_receiver_id << std::endl;
        }
    }
}

void peer_lost_callback(const char* device_id) {
    std::cout << "📤 Peer lost: " << device_id << std::endl;
}

void transfer_request_callback(const char* request_json) {
    std::cout << "📨 Transfer request: " << request_json << std::endl;
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
    transfer_completed = true;
}

void error_callback(const char* error_message) {
    std::cout << "❌ Error: " << error_message << std::endl;
}

int main() {
    std::cout << "🚀 Starting WarpDeck Sender..." << std::endl;
    
    // Initialize WarpDeck callbacks
    Callbacks callbacks = {0};
    callbacks.on_peer_discovered = peer_discovered_callback;
    callbacks.on_peer_lost = peer_lost_callback;
    callbacks.on_incoming_transfer_request = transfer_request_callback;
    callbacks.on_transfer_progress_update = transfer_progress_callback;
    callbacks.on_transfer_completed = transfer_completed_callback;
    callbacks.on_error = error_callback;
    
    WarpDeckHandle* handle = warpdeck_create(&callbacks, "/tmp/warpdeck_sender");
    if (!handle) {
        std::cerr << "❌ Failed to create WarpDeck handle" << std::endl;
        return 1;
    }
    
    // Start WarpDeck
    const char* device_name = "Sender-Device";
    
    if (!warpdeck_start(handle, device_name, 0)) {
        std::cerr << "❌ Failed to start WarpDeck" << std::endl;
        warpdeck_destroy(handle);
        return 1;
    }
    
    std::cout << "✅ WarpDeck started successfully!" << std::endl;
    std::cout << "📱 Device: " << device_name << std::endl;
    
    // Wait for peer discovery
    std::cout << "🔍 Searching for receiver device..." << std::endl;
    int discovery_timeout = 15;
    for (int i = 0; i < discovery_timeout && !peer_found; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "⏳ Waiting for peer discovery... (" << i+1 << "/" << discovery_timeout << "s)" << std::endl;
    }
    
    if (!peer_found) {
        std::cout << "⚠️  No receiver found within " << discovery_timeout << " seconds" << std::endl;
        std::cout << "🔄 Continuing to listen for 30 more seconds..." << std::endl;
        
        for (int i = 0; i < 30 && !peer_found; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (i % 5 == 0) {
                std::cout << "⏳ Still waiting... (" << i << "s)" << std::endl;
            }
        }
    }
    
    if (peer_found && !discovered_receiver_id.empty()) {
        std::cout << "🎯 Receiver found! Preparing to send files..." << std::endl;
        
        // Create files JSON for transfer
        // This is a simplified version - in a real implementation you'd use proper JSON parsing
        std::string files_json = R"([
            {
                "path": "/app/files_to_send/hello.txt",
                "name": "hello.txt"
            },
            {
                "path": "/app/files_to_send/large_file.txt", 
                "name": "large_file.txt"
            },
            {
                "path": "/app/files_to_send/timestamp.txt",
                "name": "timestamp.txt"
            }
        ])";
        
        std::cout << "📤 Initiating file transfer to device: " << discovered_receiver_id << std::endl;
        std::cout << "📁 Files to send:" << std::endl;
        
        // List files
        try {
            for (const auto& entry : std::filesystem::directory_iterator("/app/files_to_send")) {
                if (entry.is_regular_file()) {
                    std::cout << "  - " << entry.path().filename().string() 
                              << " (" << std::filesystem::file_size(entry) << " bytes)" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "  (Error listing files: " << e.what() << ")" << std::endl;
        }
        
        // Initiate transfer
        warpdeck_initiate_transfer(handle, discovered_receiver_id.c_str(), files_json.c_str());
        
        // Wait for transfer completion
        std::cout << "⏳ Waiting for transfer to complete..." << std::endl;
        int transfer_timeout = 60;
        for (int i = 0; i < transfer_timeout && !transfer_completed; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (i % 10 == 0) {
                std::cout << "⏳ Transfer in progress... (" << i << "/" << transfer_timeout << "s)" << std::endl;
            }
        }
        
        if (transfer_completed) {
            std::cout << "🎉 Transfer process completed!" << std::endl;
        } else {
            std::cout << "⚠️  Transfer timeout reached" << std::endl;
        }
        
    } else {
        std::cout << "❌ No receiver device found. Cannot initiate transfer." << std::endl;
    }
    
    // Keep running for a bit longer to see any late responses
    std::cout << "🔄 Keeping sender running for 30 more seconds..." << std::endl;
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (i % 10 == 0) {
            std::cout << "⏳ Still running... (" << i << "/30s)" << std::endl;
        }
    }
    
    // Cleanup
    warpdeck_stop(handle);
    warpdeck_destroy(handle);
    
    std::cout << "🏁 Sender test completed." << std::endl;
    return 0;
}