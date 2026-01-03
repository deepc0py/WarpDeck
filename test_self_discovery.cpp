#include "libwarpdeck/include/warpdeck.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <signal.h>

volatile bool running = true;

void signal_handler(int signal) {
    running = false;
    std::cout << "\nStopping test..." << std::endl;
}

void peer_discovered_callback(const char* peer_json) {
    std::cout << "🎉 PEER DISCOVERED: " << peer_json << std::endl;
}

void peer_lost_callback(const char* device_id) {
    std::cout << "❌ PEER LOST: " << device_id << std::endl;
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
    std::cout << "⚠️  ERROR: " << error_message << std::endl;
}

WarpDeckHandle* create_instance(const char* device_name, int port) {
    Callbacks callbacks = {0};
    callbacks.on_peer_discovered = peer_discovered_callback;
    callbacks.on_peer_lost = peer_lost_callback;
    callbacks.on_incoming_transfer_request = transfer_request_callback;
    callbacks.on_transfer_progress_update = transfer_progress_callback;
    callbacks.on_transfer_completed = transfer_completed_callback;
    callbacks.on_error = error_callback;
    
    WarpDeckHandle* handle = warpdeck_create(&callbacks, "/tmp/warpdeck");
    if (!handle) {
        std::cerr << "Failed to create WarpDeck handle for " << device_name << std::endl;
        return nullptr;
    }
    
    if (!warpdeck_start(handle, device_name, port)) {
        std::cerr << "Failed to start WarpDeck for " << device_name << std::endl;
        warpdeck_destroy(handle);
        return nullptr;
    }
    
    std::cout << "✅ Started: " << device_name << " on port " << port << std::endl;
    return handle;
}

int main() {
    signal(SIGINT, signal_handler);
    
    std::cout << "🔍 Testing Self-Discovery with Two Local Instances..." << std::endl;
    
    // Create first instance
    WarpDeckHandle* handle1 = create_instance("macOS-Device-1", 54321);
    if (!handle1) return 1;
    
    // Wait a moment
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Create second instance  
    WarpDeckHandle* handle2 = create_instance("macOS-Device-2", 54322);
    if (!handle2) {
        warpdeck_stop(handle1);
        warpdeck_destroy(handle1);
        return 1;
    }
    
    std::cout << "\n🎯 Both instances started. Waiting for mutual discovery..." << std::endl;
    
    int seconds = 0;
    while (running && seconds < 30) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        seconds++;
        
        if (seconds % 5 == 0) {
            std::cout << "\n--- Status after " << seconds << " seconds ---" << std::endl;
            
            const char* peers1 = warpdeck_get_discovered_peers(handle1);
            const char* peers2 = warpdeck_get_discovered_peers(handle2);
            
            std::cout << "Device-1 sees: " << (peers1 ? peers1 : "null") << std::endl;
            std::cout << "Device-2 sees: " << (peers2 ? peers2 : "null") << std::endl;
        }
    }
    
    // Cleanup
    std::cout << "\n🧹 Cleaning up..." << std::endl;
    warpdeck_stop(handle1);
    warpdeck_destroy(handle1);
    warpdeck_stop(handle2);
    warpdeck_destroy(handle2);
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}