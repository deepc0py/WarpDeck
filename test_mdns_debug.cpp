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
    std::cout << "Testing mDNS Debug API..." << std::endl;
    
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
    
    // Start WarpDeck
    const char* device_name = "macOS-Debug-WarpDeck";
    
    if (!warpdeck_start(handle, device_name, 0)) {
        std::cerr << "Failed to start WarpDeck" << std::endl;
        warpdeck_destroy(handle);
        return 1;
    }
    
    std::cout << "WarpDeck started successfully!" << std::endl;
    std::cout << "Device: " << device_name << std::endl;
    
    // Wait a moment for initialization
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Test debug API functions
    std::cout << "\n=== Testing Debug API ===" << std::endl;
    
    // Get discovery status
    std::cout << "\n1. Discovery Status:" << std::endl;
    const char* discovery_status = warpdeck_get_discovery_status(handle);
    if (discovery_status) {
        std::cout << discovery_status << std::endl;
    } else {
        std::cout << "Failed to get discovery status" << std::endl;
    }
    
    // Get discovered peers
    std::cout << "\n2. Discovered Peers:" << std::endl;
    const char* peers_info = warpdeck_get_discovered_peers(handle);
    if (peers_info) {
        std::cout << peers_info << std::endl;
    } else {
        std::cout << "Failed to get discovered peers" << std::endl;
    }
    
    // Get mDNS debug info
    std::cout << "\n3. mDNS Debug Info:" << std::endl;
    const char* mdns_debug = warpdeck_get_mdns_debug_info(handle);
    if (mdns_debug) {
        std::cout << mdns_debug << std::endl;
    } else {
        std::cout << "Failed to get mDNS debug info" << std::endl;
    }
    
    // Keep running for discovery
    std::cout << "\nWaiting for potential peer discovery..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "\nChecking status after " << (i + 1) << " seconds..." << std::endl;
        
        const char* updated_peers = warpdeck_get_discovered_peers(handle);
        if (updated_peers) {
            std::cout << "Current peers: " << updated_peers << std::endl;
        }
        
        const char* updated_status = warpdeck_get_discovery_status(handle);
        if (updated_status) {
            std::cout << "Discovery status: " << updated_status << std::endl;
        }
    }
    
    // Cleanup
    warpdeck_stop(handle);
    warpdeck_destroy(handle);
    
    std::cout << "\nTest completed!" << std::endl;
    return 0;
}