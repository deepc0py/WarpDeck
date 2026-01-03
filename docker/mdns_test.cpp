// Simple mDNS discovery test for WarpDeck
// Announces this device and discovers peers on the network

#include "warpdeck.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <signal.h>
#include <atomic>

std::atomic<bool> running{true};
std::atomic<int> peer_count{0};

void signal_handler(int) {
    running = false;
}

// Callback functions
void on_peer_discovered(const char* peer_json) {
    peer_count++;
    std::cout << "[DISCOVERED] Peer #" << peer_count << ": " << peer_json << std::endl;
    // Free the heap-allocated string
    warpdeck_free_string(const_cast<char*>(peer_json));
}

void on_peer_lost(const char* device_id) {
    std::cout << "[LOST] Peer: " << device_id << std::endl;
    warpdeck_free_string(const_cast<char*>(device_id));
}

void on_incoming_transfer(const char* request_json) {
    std::cout << "[TRANSFER REQUEST] " << request_json << std::endl;
    warpdeck_free_string(const_cast<char*>(request_json));
}

void on_transfer_progress(const char* transfer_id, float progress, uint64_t bytes) {
    std::cout << "[PROGRESS] " << transfer_id << ": " << progress << "% (" << bytes << " bytes)" << std::endl;
    warpdeck_free_string(const_cast<char*>(transfer_id));
}

void on_transfer_completed(const char* transfer_id, bool success, const char* error) {
    std::cout << "[COMPLETED] " << transfer_id << " - " << (success ? "SUCCESS" : "FAILED");
    if (error && strlen(error) > 0) {
        std::cout << " - " << error;
        warpdeck_free_string(const_cast<char*>(error));
    }
    std::cout << std::endl;
    warpdeck_free_string(const_cast<char*>(transfer_id));
}

void on_error(const char* error_msg) {
    std::cerr << "[ERROR] " << error_msg << std::endl;
}

int main(int argc, char* argv[]) {
    // Get device name from environment or argument
    std::string device_name = "WarpDeck-Container";
    if (argc > 1) {
        device_name = argv[1];
    } else if (const char* env_name = std::getenv("DEVICE_NAME")) {
        device_name = env_name;
    }

    // Get test duration from environment
    int test_duration = 60; // Default 60 seconds
    if (const char* env_duration = std::getenv("TEST_DURATION")) {
        test_duration = std::atoi(env_duration);
    }

    std::cout << "======================================" << std::endl;
    std::cout << "WarpDeck mDNS Discovery Test" << std::endl;
    std::cout << "Device Name: " << device_name << std::endl;
    std::cout << "Test Duration: " << test_duration << " seconds" << std::endl;
    std::cout << "======================================" << std::endl;

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Set up callbacks
    Callbacks callbacks;
    callbacks.on_peer_discovered = on_peer_discovered;
    callbacks.on_peer_lost = on_peer_lost;
    callbacks.on_incoming_transfer_request = on_incoming_transfer;
    callbacks.on_transfer_progress_update = on_transfer_progress;
    callbacks.on_transfer_completed = on_transfer_completed;
    callbacks.on_error = on_error;

    // Create config directory
    const char* config_dir = "/tmp/warpdeck";
    system("mkdir -p /tmp/warpdeck");

    std::cout << "Creating WarpDeck handle..." << std::endl;

    // Create WarpDeck instance
    WarpDeckHandle* handle = warpdeck_create(&callbacks, config_dir);
    if (!handle) {
        std::cerr << "Failed to create WarpDeck handle!" << std::endl;
        return 1;
    }

    std::cout << "Starting WarpDeck with device name: " << device_name << std::endl;

    // Start WarpDeck (port 0 = auto-select)
    int port = warpdeck_start(handle, device_name.c_str(), 0);
    if (port <= 0) {
        std::cerr << "Failed to start WarpDeck!" << std::endl;
        warpdeck_destroy(handle);
        return 1;
    }

    std::cout << "WarpDeck started on port " << port << std::endl;
    std::cout << "Listening for peers..." << std::endl;
    std::cout << "--------------------------------------" << std::endl;

    // Run for specified duration
    auto start_time = std::chrono::steady_clock::now();
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();

        if (elapsed >= test_duration) {
            std::cout << "Test duration reached." << std::endl;
            break;
        }

        // Print status every 10 seconds
        if (elapsed % 10 == 0 && elapsed > 0) {
            std::cout << "[STATUS] Elapsed: " << elapsed << "s, Peers discovered: " << peer_count << std::endl;
        }
    }

    std::cout << "--------------------------------------" << std::endl;
    std::cout << "Stopping WarpDeck..." << std::endl;

    // Clean up
    warpdeck_stop(handle);
    warpdeck_destroy(handle);

    std::cout << "======================================" << std::endl;
    std::cout << "Test Complete" << std::endl;
    std::cout << "Total peers discovered: " << peer_count << std::endl;
    std::cout << "======================================" << std::endl;

    return peer_count > 0 ? 0 : 1; // Exit 0 if any peers found
}
