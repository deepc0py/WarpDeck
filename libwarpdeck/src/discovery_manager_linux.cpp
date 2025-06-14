#ifdef WARPDECK_PLATFORM_LINUX

#include "discovery_manager.h"
#include "utils.h"
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <memory>

namespace warpdeck {

class DiscoveryManagerLinux : public DiscoveryManager::Impl {
public:
    DiscoveryManagerLinux(DiscoveryManager* parent) : parent_(parent), client_(nullptr), simple_poll_(nullptr), group_(nullptr) {}
    
    ~DiscoveryManagerLinux() {
        stop_discovery();
    }
    
    bool start_discovery(const std::string& device_name, const std::string& device_id,
                        const std::string& platform, int port, const std::string& fingerprint) override {
        // Create simple poll object
        simple_poll_ = avahi_simple_poll_new();
        if (!simple_poll_) {
            std::cerr << "Failed to create Avahi simple poll" << std::endl;
            return false;
        }
        
        // Create client
        int error;
        client_ = avahi_client_new(avahi_simple_poll_get(simple_poll_), 
                                  AVAHI_CLIENT_NO_FAIL, client_callback, this, &error);
        if (!client_) {
            std::cerr << "Failed to create Avahi client: " << avahi_strerror(error) << std::endl;
            avahi_simple_poll_free(simple_poll_);
            simple_poll_ = nullptr;
            return false;
        }
        
        device_name_ = device_name;
        device_id_ = device_id;
        platform_ = platform;
        port_ = port;
        fingerprint_ = fingerprint;
        
        // Register service and start browsing will happen in client callback
        
        // Start processing thread
        processing_thread_ = std::thread(&DiscoveryManagerLinux::process_events, this);
        
        return true;
    }
    
    void stop_discovery() override {
        running_ = false;
        
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
        
        if (group_) {
            avahi_entry_group_free(group_);
            group_ = nullptr;
        }
        
        if (client_) {
            avahi_client_free(client_);
            client_ = nullptr;
        }
        
        if (simple_poll_) {
            avahi_simple_poll_free(simple_poll_);
            simple_poll_ = nullptr;
        }
    }
    
    void update_service_info(const std::string& device_name, const std::string& device_id,
                           const std::string& platform, int port, const std::string& fingerprint) override {
        device_name_ = device_name;
        device_id_ = device_id;
        platform_ = platform;
        port_ = port;
        fingerprint_ = fingerprint;
        
        // Re-register service with new info
        register_service();
    }

private:
    static void client_callback(AvahiClient* client, AvahiClientState state, void* userdata) {
        auto* impl = static_cast<DiscoveryManagerLinux*>(userdata);
        
        switch (state) {
            case AVAHI_CLIENT_S_RUNNING:
                // Server is running, register our service
                impl->register_service();
                impl->start_browsing();
                break;
                
            case AVAHI_CLIENT_FAILURE:
                std::cerr << "Avahi client failure: " << avahi_strerror(avahi_client_errno(client)) << std::endl;
                impl->running_ = false;
                break;
                
            case AVAHI_CLIENT_S_COLLISION:
            case AVAHI_CLIENT_S_REGISTERING:
                // Server is registering, reset our group
                if (impl->group_) {
                    avahi_entry_group_reset(impl->group_);
                }
                break;
                
            case AVAHI_CLIENT_CONNECTING:
                break;
        }
    }
    
    void register_service() {
        if (!group_) {
            group_ = avahi_entry_group_new(client_, group_callback, this);
            if (!group_) {
                std::cerr << "Failed to create Avahi entry group" << std::endl;
                return;
            }
        }
        
        if (avahi_entry_group_is_empty(group_)) {
            // Create TXT record
            AvahiStringList* txt = nullptr;
            txt = avahi_string_list_add_printf(txt, "v=1.0");
            txt = avahi_string_list_add_printf(txt, "id=%s", device_id_.c_str());
            txt = avahi_string_list_add_printf(txt, "name=%s", device_name_.c_str());
            txt = avahi_string_list_add_printf(txt, "platform=%s", platform_.c_str());
            txt = avahi_string_list_add_printf(txt, "port=%d", port_);
            txt = avahi_string_list_add_printf(txt, "fp=%s", fingerprint_.c_str());
            
            int ret = avahi_entry_group_add_service_strlst(
                group_,
                AVAHI_IF_UNSPEC,
                AVAHI_PROTO_UNSPEC,
                AVAHI_PUBLISH_USE_MULTICAST,
                device_name_.c_str(),
                "_warpdeck._tcp",
                nullptr,
                nullptr,
                port_,
                txt
            );
            
            avahi_string_list_free(txt);
            
            if (ret < 0) {
                std::cerr << "Failed to add service: " << avahi_strerror(ret) << std::endl;
                return;
            }
            
            ret = avahi_entry_group_commit(group_);
            if (ret < 0) {
                std::cerr << "Failed to commit entry group: " << avahi_strerror(ret) << std::endl;
            }
        }
    }
    
    void start_browsing() {
        AvahiServiceBrowser* browser = avahi_service_browser_new(
            client_,
            AVAHI_IF_UNSPEC,
            AVAHI_PROTO_UNSPEC,
            "_warpdeck._tcp",
            nullptr,
            AVAHI_LOOKUP_USE_MULTICAST,
            browse_callback,
            this
        );
        
        if (!browser) {
            std::cerr << "Failed to create service browser" << std::endl;
        }
    }
    
    static void group_callback(AvahiEntryGroup* group, AvahiEntryGroupState state, void* userdata) {
        switch (state) {
            case AVAHI_ENTRY_GROUP_ESTABLISHED:
                std::cout << "Service registered successfully" << std::endl;
                break;
                
            case AVAHI_ENTRY_GROUP_COLLISION:
                std::cerr << "Service name collision" << std::endl;
                break;
                
            case AVAHI_ENTRY_GROUP_FAILURE:
                std::cerr << "Entry group failure" << std::endl;
                break;
                
            case AVAHI_ENTRY_GROUP_UNCOMMITED:
            case AVAHI_ENTRY_GROUP_REGISTERING:
                break;
        }
    }
    
    static void browse_callback(AvahiServiceBrowser* browser,
                              AvahiIfIndex interface,
                              AvahiProtocol protocol,
                              AvahiBrowserEvent event,
                              const char* name,
                              const char* type,
                              const char* domain,
                              AvahiLookupResultFlags flags,
                              void* userdata) {
        auto* impl = static_cast<DiscoveryManagerLinux*>(userdata);
        
        switch (event) {
            case AVAHI_BROWSER_NEW:
                // New service found, resolve it
                avahi_service_resolver_new(
                    impl->client_,
                    interface,
                    protocol,
                    name,
                    type,
                    domain,
                    AVAHI_PROTO_UNSPEC,
                    AVAHI_LOOKUP_USE_MULTICAST,
                    resolve_callback,
                    userdata
                );
                break;
                
            case AVAHI_BROWSER_REMOVE:
                // Service removed
                if (impl->parent_->peer_lost_callback_) {
                    impl->parent_->peer_lost_callback_(name);
                }
                break;
                
            case AVAHI_BROWSER_ALL_FOR_NOW:
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                break;
                
            case AVAHI_BROWSER_FAILURE:
                std::cerr << "Service browser failure" << std::endl;
                break;
        }
    }
    
    static void resolve_callback(AvahiServiceResolver* resolver,
                               AvahiIfIndex interface,
                               AvahiProtocol protocol,
                               AvahiResolverEvent event,
                               const char* name,
                               const char* type,
                               const char* domain,
                               const char* host_name,
                               const AvahiAddress* address,
                               uint16_t port,
                               AvahiStringList* txt,
                               AvahiLookupResultFlags flags,
                               void* userdata) {
        auto* impl = static_cast<DiscoveryManagerLinux*>(userdata);
        
        if (event == AVAHI_RESOLVER_FOUND) {
            // Parse TXT record
            std::map<std::string, std::string> txt_data;
            for (AvahiStringList* l = txt; l; l = l->next) {
                char* key;
                char* value;
                size_t size;
                
                if (avahi_string_list_get_pair(l, &key, &value, &size) >= 0) {
                    if (key && value) {
                        txt_data[key] = value;
                    }
                    avahi_free(key);
                    avahi_free(value);
                }
            }
            
            // Create PeerInfo
            PeerInfo peer;
            peer.id = txt_data["id"];
            peer.name = txt_data["name"];
            peer.platform = txt_data["platform"];
            peer.port = std::stoi(txt_data["port"]);
            peer.fingerprint = txt_data["fp"];
            peer.host_address = host_name;
            
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
        
        avahi_service_resolver_free(resolver);
    }
    
    void process_events() {
        while (running_ && simple_poll_) {
            avahi_simple_poll_iterate(simple_poll_, 100);
        }
    }
    
    DiscoveryManager* parent_;
    AvahiClient* client_;
    AvahiSimplePoll* simple_poll_;
    AvahiEntryGroup* group_;
    std::thread processing_thread_;
    std::atomic<bool> running_{true};
    
    std::string device_name_;
    std::string device_id_;
    std::string platform_;
    int port_;
    std::string fingerprint_;
};

void DiscoveryManager::create_platform_impl() {
    impl_ = std::make_unique<DiscoveryManagerLinux>(this);
}

} // namespace warpdeck

#endif // WARPDECK_PLATFORM_LINUX