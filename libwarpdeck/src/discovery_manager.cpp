#include "discovery_manager.h"
#include "utils.h"
#include <thread>
#include <chrono>

namespace warpdeck {

// Implementation is now defined in the header file

DiscoveryManager::DiscoveryManager() : running_(false) {
    // Platform-specific implementation will be created when needed
}

DiscoveryManager::~DiscoveryManager() {
    stop();
}

bool DiscoveryManager::start(const std::string& device_name, const std::string& device_id,
                           const std::string& platform, int port, const std::string& fingerprint) {
    if (running_) {
        return false;
    }
    
    // Create platform-specific implementation if not already created
    if (!impl_) {
        create_platform_impl();
    }
    
    device_name_ = device_name;
    device_id_ = device_id;
    platform_ = platform;
    port_ = port;
    fingerprint_ = fingerprint;
    
    if (!impl_ || !impl_->start_discovery(device_name, device_id, platform, port, fingerprint)) {
        return false;
    }
    
    running_ = true;
    discovery_thread_ = std::thread(&DiscoveryManager::discovery_thread_func, this);
    
    return true;
}

// create_platform_impl() is implemented in platform-specific files

void DiscoveryManager::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (impl_) {
        impl_->stop_discovery();
    }
    
    if (discovery_thread_.joinable()) {
        discovery_thread_.join();
    }
    
    std::lock_guard<std::mutex> lock(peers_mutex_);
    discovered_peers_.clear();
}

void DiscoveryManager::set_device_name(const std::string& name) {
    device_name_ = name;
    if (running_ && impl_) {
        impl_->update_service_info(device_name_, device_id_, platform_, port_, fingerprint_);
    }
}

std::map<std::string, PeerInfo> DiscoveryManager::get_discovered_peers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return discovered_peers_;
}

void DiscoveryManager::set_peer_discovered_callback(PeerDiscoveredCallback callback) {
    peer_discovered_callback_ = callback;
}

void DiscoveryManager::set_peer_lost_callback(PeerLostCallback callback) {
    peer_lost_callback_ = callback;
}

void DiscoveryManager::discovery_thread_func() {
    // This thread will handle periodic discovery updates
    // The actual mDNS discovery is handled by the platform-specific implementation
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Periodic cleanup of stale peers could be done here
        // For now, we rely on the platform-specific implementation to handle this
    }
}

void DiscoveryManager::update_service_registration() {
    if (impl_) {
        impl_->update_service_info(device_name_, device_id_, platform_, port_, fingerprint_);
    }
}

} // namespace warpdeck