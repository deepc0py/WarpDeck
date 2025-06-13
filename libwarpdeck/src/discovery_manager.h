#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace warpdeck {

struct PeerInfo {
    std::string id;
    std::string name;
    std::string platform;
    int port;
    std::string fingerprint;
    std::string host_address;
};

class DiscoveryManager {
public:
    using PeerDiscoveredCallback = std::function<void(const PeerInfo&)>;
    using PeerLostCallback = std::function<void(const std::string& device_id)>;

    DiscoveryManager();
    ~DiscoveryManager();

    bool start(const std::string& device_name, const std::string& device_id, 
               const std::string& platform, int port, const std::string& fingerprint);
    void stop();
    
    void set_device_name(const std::string& name);
    
    std::map<std::string, PeerInfo> get_discovered_peers() const;
    
    void set_peer_discovered_callback(PeerDiscoveredCallback callback);
    void set_peer_lost_callback(PeerLostCallback callback);

    // Platform-specific implementation base class
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual bool start_discovery(const std::string& device_name, const std::string& device_id,
                                    const std::string& platform, int port, const std::string& fingerprint) = 0;
        virtual void stop_discovery() = 0;
        virtual void update_service_info(const std::string& device_name, const std::string& device_id,
                                       const std::string& platform, int port, const std::string& fingerprint) = 0;
    };
    std::unique_ptr<Impl> impl_;
    
    void create_platform_impl();

private:
    void discovery_thread_func();
    void update_service_registration();
    
    std::atomic<bool> running_;
    std::thread discovery_thread_;
    
    // Service registration info
    std::string device_name_;
    std::string device_id_;
    std::string platform_;
    int port_;
    std::string fingerprint_;
    
public:
    // These need to be accessible by platform implementations
    mutable std::mutex peers_mutex_;
    std::map<std::string, PeerInfo> discovered_peers_;
    PeerDiscoveredCallback peer_discovered_callback_;
    PeerLostCallback peer_lost_callback_;
};

} // namespace warpdeck