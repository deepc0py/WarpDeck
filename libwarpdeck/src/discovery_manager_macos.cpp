#ifdef WARPDECK_PLATFORM_MACOS

#include "discovery_manager.h"
#include "utils.h"
#include <dns_sd.h>
#include <CoreFoundation/CoreFoundation.h>
#include <map>
#include <string>
#include <thread>
#include <iostream>

namespace warpdeck {

class DiscoveryManagerMacOSImpl : public DiscoveryManager::Impl {
public:
    DiscoveryManagerMacOSImpl(DiscoveryManager* parent) : parent_(parent), service_ref_(nullptr), browse_ref_(nullptr) {}
    
    ~DiscoveryManagerMacOSImpl() {
        stop_discovery();
    }
    
    bool start_discovery(const std::string& device_name, const std::string& device_id,
                        const std::string& platform, int port, const std::string& fingerprint) override {
        // Store our device ID for self-filtering
        device_id_ = device_id;
        
        // Create TXT record
        std::map<std::string, std::string> txt_record;
        txt_record["v"] = "1.0";
        txt_record["id"] = device_id;
        txt_record["name"] = device_name;
        txt_record["platform"] = platform;
        txt_record["port"] = std::to_string(port);
        txt_record["fp"] = fingerprint;
        
        // Convert to DNS-SD TXT record format
        std::vector<uint8_t> txt_data;
        for (const auto& [key, value] : txt_record) {
            std::string entry = key + "=" + value;
            txt_data.push_back(static_cast<uint8_t>(entry.length()));
            txt_data.insert(txt_data.end(), entry.begin(), entry.end());
        }
        
        // Register service
        DNSServiceErrorType error = DNSServiceRegister(
            &service_ref_,
            0,                          // flags
            kDNSServiceInterfaceIndexAny, // interface index
            device_name.c_str(),        // service name
            "_warpdeck._tcp",           // service type
            nullptr,                    // domain (default)
            nullptr,                    // host (default)
            htons(port),               // port
            txt_data.size(),           // TXT record length
            txt_data.data(),           // TXT record data
            register_callback,         // callback
            this                       // context
        );
        
        if (error != kDNSServiceErr_NoError) {
            std::cerr << "Failed to register service: " << error << std::endl;
            return false;
        }
        
        // Start browsing for other services
        error = DNSServiceBrowse(
            &browse_ref_,
            0,                          // flags
            kDNSServiceInterfaceIndexAny, // interface index
            "_warpdeck._tcp",           // service type
            nullptr,                    // domain (default)
            browse_callback,           // callback
            this                       // context
        );
        
        if (error != kDNSServiceErr_NoError) {
            std::cerr << "Failed to start browsing: " << error << std::endl;
            DNSServiceRefDeallocate(service_ref_);
            service_ref_ = nullptr;
            return false;
        }
        
        // Start processing thread
        processing_thread_ = std::thread(&DiscoveryManagerMacOSImpl::process_events, this);
        
        return true;
    }
    
    void stop_discovery() override {
        running_ = false;
        
        if (service_ref_) {
            DNSServiceRefDeallocate(service_ref_);
            service_ref_ = nullptr;
        }
        
        if (browse_ref_) {
            DNSServiceRefDeallocate(browse_ref_);
            browse_ref_ = nullptr;
        }
        
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
        
        // Clean up resolve operations
        for (auto& [name, ref] : resolve_refs_) {
            if (ref) {
                DNSServiceRefDeallocate(ref);
            }
        }
        resolve_refs_.clear();
    }
    
    void update_service_info(const std::string& device_name, const std::string& device_id,
                           const std::string& platform, int port, const std::string& fingerprint) override {
        // For simplicity, we'll stop and restart the service
        // A more sophisticated implementation would use DNSServiceUpdateRecord
        if (service_ref_) {
            DNSServiceRefDeallocate(service_ref_);
            service_ref_ = nullptr;
        }
        
        // Re-register with new info
        start_discovery(device_name, device_id, platform, port, fingerprint);
    }

private:
    static void register_callback(DNSServiceRef /* service */,
                                DNSServiceFlags /* flags */,
                                DNSServiceErrorType errorCode,
                                const char* name,
                                const char* /* regtype */,
                                const char* /* domain */,
                                void* /* context */) {
        if (errorCode == kDNSServiceErr_NoError) {
            std::cout << "Service registered: " << name << std::endl;
        } else {
            std::cerr << "Service registration failed: " << errorCode << std::endl;
        }
    }
    
    static void browse_callback(DNSServiceRef /* service */,
                              DNSServiceFlags flags,
                              uint32_t interfaceIndex,
                              DNSServiceErrorType errorCode,
                              const char* serviceName,
                              const char* regtype,
                              const char* replyDomain,
                              void* context) {
        auto* impl = static_cast<DiscoveryManagerMacOSImpl*>(context);
        
        if (errorCode != kDNSServiceErr_NoError) {
            return;
        }
        
        if (flags & kDNSServiceFlagsAdd) {
            // New service found, start resolving it
            DNSServiceRef resolve_ref;
            DNSServiceErrorType error = DNSServiceResolve(
                &resolve_ref,
                0,                      // flags
                interfaceIndex,
                serviceName,
                regtype,
                replyDomain,
                resolve_callback,
                context
            );
            
            if (error == kDNSServiceErr_NoError) {
                impl->resolve_refs_[serviceName] = resolve_ref;
            }
        } else {
            // Service removed
            auto it = impl->resolve_refs_.find(serviceName);
            if (it != impl->resolve_refs_.end()) {
                DNSServiceRefDeallocate(it->second);
                impl->resolve_refs_.erase(it);
            }
            
            // Notify about peer loss
            if (impl->parent_->peer_lost_callback_) {
                impl->parent_->peer_lost_callback_(serviceName);
            }
        }
    }
    
    static void resolve_callback(DNSServiceRef /* service */,
                               DNSServiceFlags /* flags */,
                               uint32_t /* interfaceIndex */,
                               DNSServiceErrorType errorCode,
                               const char* /* fullname */,
                               const char* hosttarget,
                               uint16_t /* port */,
                               uint16_t txtLen,
                               const unsigned char* txtRecord,
                               void* context) {
        if (!context) {
            std::cerr << "Null context in resolve_callback" << std::endl;
            return;
        }
        auto* impl = static_cast<DiscoveryManagerMacOSImpl*>(context);
        
        if (errorCode != kDNSServiceErr_NoError) {
            return;
        }
        
        // Parse TXT record
        std::map<std::string, std::string> txt_data;
        const unsigned char* ptr = txtRecord;
        const unsigned char* end = txtRecord + txtLen;
        
        while (ptr < end) {
            uint8_t len = *ptr++;
            if (ptr + len > end) break;
            
            std::string entry(reinterpret_cast<const char*>(ptr), len);
            ptr += len;
            
            size_t eq_pos = entry.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = entry.substr(0, eq_pos);
                std::string value = entry.substr(eq_pos + 1);
                txt_data[key] = value;
            }
        }
        
        // Validate required fields before creating PeerInfo
        const std::vector<std::string> required_fields = {"id", "name", "platform", "port", "fp"};
        for (const std::string& field : required_fields) {
            if (txt_data.find(field) == txt_data.end() || txt_data[field].empty()) {
                std::cerr << "Missing required field in TXT record: " << field << std::endl;
                return; // Skip this peer
            }
        }
        
        // Parse port with error handling
        int parsed_port = 0;
        try {
            parsed_port = std::stoi(txt_data["port"]);
            if (parsed_port <= 0 || parsed_port > 65535) {
                std::cerr << "Invalid port number: " << parsed_port << std::endl;
                return; // Skip this peer
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse port: " << txt_data["port"] << " - " << e.what() << std::endl;
            return; // Skip this peer
        }
        
        // Create PeerInfo
        PeerInfo peer;
        peer.id = txt_data["id"];
        peer.name = txt_data["name"];
        peer.platform = txt_data["platform"];
        peer.port = parsed_port;
        peer.fingerprint = txt_data["fp"];
        peer.host_address = hosttarget;
        
        // Skip if this is our own device (self-filtering)
        if (peer.id == impl->device_id_) {
            std::cout << "Skipping self-discovery: " << peer.id << std::endl;
            return;
        }
        
        // Add to discovered peers
        {
            std::lock_guard<std::mutex> lock(impl->parent_->peers_mutex_);
            impl->parent_->discovered_peers_[peer.id] = peer;
        }
        
        // Notify callback
        if (impl->parent_->peer_discovered_callback_) {
            impl->parent_->peer_discovered_callback_(peer);
        }
    }
    
    void process_events() {
        while (running_) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            
            int max_fd = 0;
            
            if (service_ref_) {
                int fd = DNSServiceRefSockFD(service_ref_);
                FD_SET(fd, &read_fds);
                max_fd = std::max(max_fd, fd);
            }
            
            if (browse_ref_) {
                int fd = DNSServiceRefSockFD(browse_ref_);
                FD_SET(fd, &read_fds);
                max_fd = std::max(max_fd, fd);
            }
            
            for (const auto& [name, ref] : resolve_refs_) {
                if (ref) {
                    int fd = DNSServiceRefSockFD(ref);
                    FD_SET(fd, &read_fds);
                    max_fd = std::max(max_fd, fd);
                }
            }
            
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
            
            if (result > 0) {
                if (service_ref_ && FD_ISSET(DNSServiceRefSockFD(service_ref_), &read_fds)) {
                    DNSServiceProcessResult(service_ref_);
                }
                
                if (browse_ref_ && FD_ISSET(DNSServiceRefSockFD(browse_ref_), &read_fds)) {
                    DNSServiceProcessResult(browse_ref_);
                }
                
                for (const auto& [name, ref] : resolve_refs_) {
                    if (ref && FD_ISSET(DNSServiceRefSockFD(ref), &read_fds)) {
                        DNSServiceProcessResult(ref);
                    }
                }
            }
        }
    }
    
    DiscoveryManager* parent_;
    DNSServiceRef service_ref_;
    DNSServiceRef browse_ref_;
    std::map<std::string, DNSServiceRef> resolve_refs_;
    std::thread processing_thread_;
    std::atomic<bool> running_{true};
    std::string device_id_;
};


void DiscoveryManager::create_platform_impl() {
    impl_ = std::make_unique<DiscoveryManagerMacOSImpl>(this);
}

} // namespace warpdeck

#endif // WARPDECK_PLATFORM_MACOS