#include "libwarpdeck/include/warpdeck.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>

void peer_discovered_callback(const char* peer_json) {
    std::cout << "Peer discovered: " << peer_json << std::endl;
}

void peer_lost_callback(const char* device_id) {
    std::cout << "Peer lost: " << device_id << std::endl;
}

void transfer_request_callback(const char* request_json) {
    std::cout << "Transfer request: " << request_json << std::endl;
}

void transfer_progress_callback(const char* transfer_id, float progress_percent, uint64_t bytes_transferred) {
    std::cout << "Transfer progress - ID: " << transfer_id << ", Progress: " << progress_percent << "%, Bytes: " << bytes_transferred << std::endl;
}

void transfer_completed_callback(const char* transfer_id, bool success, const char* error_message) {
    std::cout << "Transfer completed - ID: " << transfer_id << ", Success: " << (success ? "yes" : "no");
    if (error_message) {
        std::cout << ", Error: " << error_message;
    }
    std::cout << std::endl;
}

void error_callback(const char* error_message) {
    std::cout << "Error: " << error_message << std::endl;
}

int main() {
    std::cout << "Starting WarpDeck test..." << std::endl;
    
    // Initialize WarpDeck callbacks
    Callbacks callbacks = {0};
    callbacks.on_peer_discovered = peer_discovered_callback;
    callbacks.on_peer_lost = peer_lost_callback;
    callbacks.on_incoming_transfer_request = transfer_request_callback;
    callbacks.on_transfer_progress_update = transfer_progress_callback;
    callbacks.on_transfer_completed = transfer_completed_callback;
    callbacks.on_error = error_callback;
    
    WarpDeckHandle* handle = warpdeck_create(&callbacks, "/tmp/warpdeck");
    if (!handle) {
        std::cerr << "Failed to create WarpDeck handle" << std::endl;
        return 1;
    }
    
    // Start WarpDeck with Linux device info
    const char* device_name = "Linux-Docker-WarpDeck";
    
    if (!warpdeck_start(handle, device_name, 0)) {
        std::cerr << "Failed to start WarpDeck" << std::endl;
        warpdeck_destroy(handle);
        return 1;
    }
    
    std::cout << "WarpDeck started successfully!" << std::endl;
    std::cout << "Device: " << device_name << std::endl;
    
    // Wait and let discovery happen through callbacks
    std::cout << "Waiting for peer discovery..." << std::endl;
    for (int i = 0; i < 30; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Waiting... (" << i+1 << "s)" << std::endl;
    }
    
    // Cleanup
    warpdeck_stop(handle);
    warpdeck_destroy(handle);
    
    std::cout << "WarpDeck test completed." << std::endl;
    return 0;
}