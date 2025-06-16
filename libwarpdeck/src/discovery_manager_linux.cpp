#ifdef WARPDECK_PLATFORM_LINUX

#include "discovery_manager.h"
#include "utils.h"
#include "logger.h"
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
#include <chrono>
#include <algorithm>

namespace warpdeck {

class DiscoveryManagerLinux : public DiscoveryManager::Impl {
public:
    DiscoveryManagerLinux(DiscoveryManager* parent) : parent_(parent), client_(nullptr), simple_poll_(nullptr), group_(nullptr),
                         reconnect_attempts_(0), max_reconnect_attempts_(10), base_reconnect_delay_ms_(1000) {
        LOG_DISCOVERY_INFO() << "Creating Linux discovery manager";
    }
    
    ~DiscoveryManagerLinux() {
        LOG_DISCOVERY_INFO() << "Destroying Linux discovery manager";
        stop_discovery();
    }
    
    bool start_discovery(const std::string& device_name, const std::string& device_id,
                        const std::string& platform, int port, const std::string& fingerprint) override {
        LOG_DISCOVERY_INFO() << "Starting Linux discovery for device: " << device_name << " (ID: " << device_id << ")";
        LOG_DISCOVERY_DEBUG() << "Platform: " << platform << ", Port: " << port;
        LOG_DISCOVERY_DEBUG() << "Fingerprint: " << fingerprint.substr(0, 16) << "...";
        
        // Create simple poll object
        simple_poll_ = avahi_simple_poll_new();
        if (!simple_poll_) {
            LOG_DISCOVERY_ERROR() << "Failed to create Avahi simple poll";
            return false;
        }
        
        // Create client
        int error;
        LOG_DISCOVERY_DEBUG() << "Creating Avahi client...";
        client_ = avahi_client_new(avahi_simple_poll_get(simple_poll_), 
                                  AVAHI_CLIENT_NO_FAIL, client_callback, this, &error);
        if (!client_) {
            LOG_DISCOVERY_ERROR() << "Failed to create Avahi client: " << avahi_strerror(error);
            avahi_simple_poll_free(simple_poll_);
            simple_poll_ = nullptr;
            return false;
        }
        LOG_DISCOVERY_INFO() << "Successfully created Avahi client";
        
        device_name_ = device_name;
        device_id_ = device_id;
        platform_ = platform;
        port_ = port;
        fingerprint_ = fingerprint;
        
        // Register service and start browsing will happen in client callback
        
        // Start processing thread
        LOG_DISCOVERY_DEBUG() << "Starting Avahi event processing thread";
        processing_thread_ = std::thread(&DiscoveryManagerLinux::process_events, this);
        
        LOG_DISCOVERY_INFO() << "Successfully started Linux discovery service";
        return true;
    }
    
    void stop_discovery() override {
        LOG_DISCOVERY_INFO() << "Stopping Linux discovery service";
        running_ = false;
        
        if (processing_thread_.joinable()) {
            LOG_DISCOVERY_DEBUG() << "Waiting for processing thread to finish";
            processing_thread_.join();
        }
        
        if (group_) {
            LOG_DISCOVERY_DEBUG() << "Freeing Avahi entry group";
            avahi_entry_group_free(group_);
            group_ = nullptr;
        }
        
        if (client_) {
            LOG_DISCOVERY_DEBUG() << "Freeing Avahi client";
            avahi_client_free(client_);
            client_ = nullptr;
        }
        
        if (simple_poll_) {
            LOG_DISCOVERY_DEBUG() << "Freeing Avahi simple poll";
            avahi_simple_poll_free(simple_poll_);
            simple_poll_ = nullptr;
        }
        
        LOG_DISCOVERY_INFO() << "Linux discovery service stopped";
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
    void schedule_reconnect() {
        if (reconnect_attempts_ >= max_reconnect_attempts_) {
            LOG_DISCOVERY_ERROR() << "Max reconnection attempts reached (" << max_reconnect_attempts_ << "), giving up";
            running_ = false;
            return;
        }
        
        if (reconnecting_.exchange(true)) {
            return; // Already reconnecting
        }
        
        reconnect_attempts_++;
        
        // Calculate exponential backoff delay: base_delay * 2^(attempts-1)
        int delay_ms = base_reconnect_delay_ms_ * (1 << (reconnect_attempts_ - 1));
        // Cap the delay at 30 seconds
        delay_ms = std::min(delay_ms, 30000);
        
        LOG_DISCOVERY_WARN() << "Scheduling reconnection attempt " << reconnect_attempts_ 
                            << "/" << max_reconnect_attempts_ << " in " << delay_ms << "ms";
        
        // Schedule reconnection in a separate thread
        std::thread([this, delay_ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            if (running_) {
                reconnect();
            }
        }).detach();
    }
    
    void reconnect() {
        LOG_DISCOVERY_INFO() << "Attempting to reconnect to Avahi daemon...";
        
        // Clean up existing client
        if (group_) {
            avahi_entry_group_free(group_);
            group_ = nullptr;
        }
        
        if (client_) {
            avahi_client_free(client_);
            client_ = nullptr;
        }
        
        // Try to create a new client
        int error;
        client_ = avahi_client_new(avahi_simple_poll_get(simple_poll_), 
                                  AVAHI_CLIENT_NO_FAIL, client_callback, this, &error);
        
        if (!client_) {
            LOG_DISCOVERY_ERROR() << "Reconnection failed: " << avahi_strerror(error);
            reconnecting_ = false;
            schedule_reconnect(); // Try again
        } else {
            LOG_DISCOVERY_INFO() << "Successfully reconnected to Avahi daemon";
            reconnect_attempts_ = 0; // Reset counter on successful reconnection
            reconnecting_ = false;
        }
    }

    static void client_callback(AvahiClient* client, AvahiClientState state, void* userdata) {
        auto* impl = static_cast<DiscoveryManagerLinux*>(userdata);
        
        switch (state) {
            case AVAHI_CLIENT_S_RUNNING:
                LOG_DISCOVERY_INFO() << "Avahi client is running, registering service and starting browsing";
                // Server is running, register our service
                impl->register_service();
                impl->start_browsing();
                break;
                
            case AVAHI_CLIENT_FAILURE:
                LOG_DISCOVERY_ERROR() << "Avahi client failure: " << avahi_strerror(avahi_client_errno(client));
                impl->schedule_reconnect();
                break;
                
            case AVAHI_CLIENT_S_COLLISION:
                LOG_DISCOVERY_WARN() << "Avahi client collision detected";
                // Fall through
            case AVAHI_CLIENT_S_REGISTERING:
                LOG_DISCOVERY_DEBUG() << "Avahi client is registering, resetting group";
                // Server is registering, reset our group
                if (impl->group_) {
                    avahi_entry_group_reset(impl->group_);
                }
                break;
                
            case AVAHI_CLIENT_CONNECTING:
                LOG_DISCOVERY_DEBUG() << "Avahi client is connecting to daemon";
                break;
        }
    }
    
    void register_service() {
        LOG_DISCOVERY_DEBUG() << "Registering Avahi service: " << device_name_;
        if (!group_) {
            group_ = avahi_entry_group_new(client_, group_callback, this);
            if (!group_) {
                LOG_DISCOVERY_ERROR() << "Failed to create Avahi entry group";
                return;
            }
            LOG_DISCOVERY_DEBUG() << "Created Avahi entry group";
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
                LOG_DISCOVERY_ERROR() << "Failed to add service: " << avahi_strerror(ret);
                return;
            }
            LOG_DISCOVERY_DEBUG() << "Successfully added service to entry group";
            
            ret = avahi_entry_group_commit(group_);
            if (ret < 0) {
                LOG_DISCOVERY_ERROR() << "Failed to commit entry group: " << avahi_strerror(ret);
            } else {
                LOG_DISCOVERY_DEBUG() << "Successfully committed entry group";
            }
        }
    }
    
    void start_browsing() {
        LOG_DISCOVERY_INFO() << "Starting Avahi service browsing for _warpdeck._tcp";
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
            LOG_DISCOVERY_ERROR() << "Failed to create service browser";
        } else {
            LOG_DISCOVERY_INFO() << "Successfully started service browsing";
        }
    }
    
    static void group_callback(AvahiEntryGroup* /* group */, AvahiEntryGroupState state, void* /* userdata */) {
        switch (state) {
            case AVAHI_ENTRY_GROUP_ESTABLISHED:
                LOG_DISCOVERY_INFO() << "Avahi service registered successfully";
                break;
                
            case AVAHI_ENTRY_GROUP_COLLISION:
                LOG_DISCOVERY_ERROR() << "Service name collision detected";
                break;
                
            case AVAHI_ENTRY_GROUP_FAILURE:
                LOG_DISCOVERY_ERROR() << "Avahi entry group failure";
                break;
                
            case AVAHI_ENTRY_GROUP_UNCOMMITED:
                LOG_DISCOVERY_DEBUG() << "Entry group uncommitted";
                break;
            case AVAHI_ENTRY_GROUP_REGISTERING:
                LOG_DISCOVERY_DEBUG() << "Entry group registering";
                break;
        }
    }
    
    static void browse_callback(AvahiServiceBrowser* /* browser */,
                              AvahiIfIndex interface,
                              AvahiProtocol protocol,
                              AvahiBrowserEvent event,
                              const char* name,
                              const char* type,
                              const char* domain,
                              AvahiLookupResultFlags /* flags */,
                              void* userdata) {
        auto* impl = static_cast<DiscoveryManagerLinux*>(userdata);
        
        switch (event) {
            case AVAHI_BROWSER_NEW:
                LOG_DISCOVERY_INFO() << "Discovered new service: " << name;
                LOG_DISCOVERY_DEBUG() << "Service type: " << type << ", Domain: " << domain;
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
                LOG_DISCOVERY_INFO() << "Service removed: " << name;
                // Service removed
                if (impl->parent_->peer_lost_callback_) {
                    impl->parent_->peer_lost_callback_(name);
                }
                break;
                
            case AVAHI_BROWSER_ALL_FOR_NOW:
                LOG_DISCOVERY_DEBUG() << "Browse: All for now";
                break;
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                LOG_DISCOVERY_DEBUG() << "Browse: Cache exhausted";
                break;
                
            case AVAHI_BROWSER_FAILURE:
                LOG_DISCOVERY_ERROR() << "Service browser failure";
                break;
        }
    }
    
    static void resolve_callback(AvahiServiceResolver* resolver,
                               AvahiIfIndex /* interface */,
                               AvahiProtocol /* protocol */,
                               AvahiResolverEvent event,
                               const char* /* name */,
                               const char* /* type */,
                               const char* /* domain */,
                               const char* host_name,
                               const AvahiAddress* /* address */,
                               uint16_t /* port */,
                               AvahiStringList* txt,
                               AvahiLookupResultFlags /* flags */,
                               void* userdata) {
        auto* impl = static_cast<DiscoveryManagerLinux*>(userdata);
        
        if (event == AVAHI_RESOLVER_FOUND) {
            LOG_DISCOVERY_DEBUG() << "Resolving service, host: " << host_name;
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
            
            // Validate required fields before creating PeerInfo
            const std::vector<std::string> required_fields = {"id", "name", "platform", "port", "fp"};
            for (const std::string& field : required_fields) {
                if (txt_data.find(field) == txt_data.end() || txt_data[field].empty()) {
                    LOG_DISCOVERY_WARN() << "Missing required field in TXT record: " << field;
                    return; // Skip this peer
                }
            }
            
            // Parse port with error handling
            int parsed_port = 0;
            try {
                parsed_port = std::stoi(txt_data["port"]);
                if (parsed_port <= 0 || parsed_port > 65535) {
                    LOG_DISCOVERY_WARN() << "Invalid port number: " << parsed_port;
                    return; // Skip this peer
                }
            } catch (const std::exception& e) {
                LOG_DISCOVERY_WARN() << "Failed to parse port: " << txt_data["port"] << " - " << e.what();
                return; // Skip this peer
            }
            
            // Create PeerInfo
            PeerInfo peer;
            peer.id = txt_data["id"];
            peer.name = txt_data["name"];
            peer.platform = txt_data["platform"];
            peer.port = parsed_port;
            peer.fingerprint = txt_data["fp"];
            peer.host_address = host_name;
            
            // Skip if this is our own device (self-filtering)
            if (peer.id == impl->device_id_) {
                LOG_DISCOVERY_DEBUG() << "Skipping self-discovery for device: " << peer.id;
                return;
            }
            
            LOG_DISCOVERY_INFO() << "Successfully resolved peer: " << peer.name 
                                << " (" << peer.id << ") at " << peer.host_address 
                                << ":" << peer.port << " [" << peer.platform << "]";
            
            // Add to discovered peers
            {
                std::lock_guard<std::mutex> lock(impl->parent_->peers_mutex_);
                impl->parent_->discovered_peers_[peer.id] = peer;
            }
            
            // Notify callback
            if (impl->parent_->peer_discovered_callback_) {
                impl->parent_->peer_discovered_callback_(peer);
            }
        } else {
            LOG_DISCOVERY_DEBUG() << "Service resolution failed or incomplete";
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
    
    // Reconnection logic
    std::atomic<int> reconnect_attempts_;
    const int max_reconnect_attempts_;
    const int base_reconnect_delay_ms_;
    std::atomic<bool> reconnecting_{false};
};

void DiscoveryManager::create_platform_impl() {
    impl_ = std::make_unique<DiscoveryManagerLinux>(this);
}

} // namespace warpdeck

#endif // WARPDECK_PLATFORM_LINUX