#include "warpdeck.h"
#include "discovery_manager.h"
#include "mdns_manager.h"
#include "api_server.h"
#include "api_client.h"
#include "security_manager.h"
#include "transfer_manager.h"
#include "utils.h"
#include "logger.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <cstring>
#include <map>

using namespace warpdeck;

struct WarpDeckHandle {
    std::unique_ptr<DiscoveryManager> discovery_manager;
    std::unique_ptr<APIServer> api_server;
    std::unique_ptr<APIClient> api_client;
    std::unique_ptr<SecurityManager> security_manager;
    std::unique_ptr<TransferManager> transfer_manager;
    
    Callbacks callbacks;
    std::string device_id;
    std::string device_name;
    std::string config_dir;
    int current_port;
    bool started;
    
    WarpDeckHandle() : current_port(0), started(false) {}
};

// Helper function to safely call callbacks
template<typename Callback, typename... Args>
void safe_call_callback(Callback callback, Args... args) {
    if (callback) {
        try {
            callback(args...);
        } catch (...) {
            // Ignore callback exceptions to prevent them from propagating back to C++ core
        }
    }
}

// Helper function to copy string for C API
char* copy_string(const std::string& str) {
    char* result = new char[str.length() + 1];
    std::strcpy(result, str.c_str());
    return result;
}

extern "C" {

WarpDeckHandle* warpdeck_create(const Callbacks* callbacks, const char* config_dir) {
    if (!callbacks || !config_dir) {
        return nullptr;
    }
    
    try {
        auto handle = std::make_unique<WarpDeckHandle>();
        
        // Copy callbacks
        handle->callbacks = *callbacks;
        handle->config_dir = config_dir;
        handle->device_id = utils::generate_uuid();
        
        // Initialize managers
        handle->discovery_manager = std::make_unique<DiscoveryManager>();
        handle->api_server = std::make_unique<APIServer>();
        handle->api_client = std::make_unique<APIClient>();
        handle->security_manager = std::make_unique<SecurityManager>();
        handle->transfer_manager = std::make_unique<TransferManager>();
        
        // Initialize security manager
        if (!handle->security_manager->initialize(config_dir)) {
            return nullptr;
        }
        
        // Generate certificate if needed
        if (!handle->security_manager->generate_certificate_if_needed()) {
            return nullptr;
        }
        
        // Set up discovery manager callbacks
        // NOTE: We use copy_string() (new char[]) to allocate heap memory for callback strings
        // because NativeCallable.listener in Dart processes callbacks asynchronously.
        // By the time Dart reads the string, the original std::string may be destroyed.
        // Dart must call warpdeck_free_string() (which uses delete[]) after processing.
        handle->discovery_manager->set_peer_discovered_callback(
            [handle = handle.get()](const PeerInfo& peer) {
                LOG_CORE_INFO() << "Peer discovered: " << peer.name << " (ID: " << peer.id << ")";
                std::string json = utils::peer_info_to_json(peer);
                char* json_copy = copy_string(json);  // Heap allocated with new[], Dart must call warpdeck_free_string()
                safe_call_callback(handle->callbacks.on_peer_discovered, json_copy);
            });

        handle->discovery_manager->set_peer_lost_callback(
            [handle = handle.get()](const std::string& device_id) {
                LOG_CORE_INFO() << "Peer lost: " << device_id;
                char* id_copy = copy_string(device_id);  // Heap allocated with new[], Dart must call warpdeck_free_string()
                safe_call_callback(handle->callbacks.on_peer_lost, id_copy);
            });
        
        // Set up transfer manager callbacks (using copy_string for async Dart callbacks)
        handle->transfer_manager->set_progress_callback(
            [handle = handle.get()](const std::string& transfer_id, float progress, uint64_t bytes) {
                char* id_copy = copy_string(transfer_id);  // Heap allocated with new[], Dart must call warpdeck_free_string()
                safe_call_callback(handle->callbacks.on_transfer_progress_update,
                                 id_copy, progress, bytes);
            });

        handle->transfer_manager->set_completion_callback(
            [handle = handle.get()](const std::string& transfer_id, bool success, const std::string& error) {
                char* id_copy = copy_string(transfer_id);  // Heap allocated with new[], Dart must call warpdeck_free_string()
                char* error_copy = error.empty() ? nullptr : copy_string(error);  // Heap allocated with new[] if not empty
                safe_call_callback(handle->callbacks.on_transfer_completed,
                                 id_copy, success, error_copy);
            });

        handle->transfer_manager->set_incoming_request_callback(
            [handle = handle.get()](const std::string& transfer_id, const std::string& peer_name,
                                   const std::vector<FileMetadata>& files) {
                // Create JSON for the transfer request
                TransferRequest request;
                request.files = files;
                std::string json = utils::transfer_request_to_json(request);
                char* json_copy = copy_string(json);  // Heap allocated with new[], Dart must call warpdeck_free_string()
                safe_call_callback(handle->callbacks.on_incoming_transfer_request, json_copy);
            });

        // Set up transfer manager dependencies for sending
        handle->transfer_manager->set_api_client(handle->api_client.get());
        handle->transfer_manager->set_peer_lookup(
            [handle = handle.get()](const std::string& device_id) -> PeerConnectionInfo {
                auto peers = handle->discovery_manager->get_discovered_peers();
                auto it = peers.find(device_id);
                if (it != peers.end()) {
                    PeerConnectionInfo info;
                    info.host = it->second.host_address;
                    info.port = it->second.port;
                    info.fingerprint = it->second.fingerprint;
                    return info;
                }
                return PeerConnectionInfo{};  // Empty info if not found
            });

        // Set up API server callbacks
        handle->api_server->set_transfer_request_callback(
            [handle = handle.get()](const std::string& client_fingerprint,
                                   const TransferRequest& request,
                                   std::function<void(bool, const std::string&)> response_callback) {
                // Try to find peer info by fingerprint from discovered peers
                std::string peer_id = client_fingerprint;
                std::string peer_name = "Unknown Peer";
                bool is_trusted = false;

                // Look up peer by fingerprint in discovered peers
                auto peers = handle->discovery_manager->get_discovered_peers();
                for (const auto& [device_id, peer] : peers) {
                    if (peer.fingerprint == client_fingerprint) {
                        peer_id = peer.id;
                        peer_name = peer.name;
                        break;
                    }
                }

                // Check if peer is in trust store
                is_trusted = handle->security_manager->is_peer_trusted(peer_id, client_fingerprint);

                // Handle the incoming request through transfer manager
                std::string transfer_id = handle->transfer_manager->handle_incoming_request(
                    peer_id, peer_name, request);

                if (!is_trusted) {
                    // Will trigger the incoming request callback to UI
                    response_callback(false, ""); // Will be handled by respond_to_transfer
                } else {
                    response_callback(true, transfer_id);
                }
            });
            
        handle->api_server->set_file_upload_callback(
            [handle = handle.get()](const std::string& transfer_id, int file_index, 
                                   const std::string& data,
                                   std::function<void(bool, const std::string&)> response_callback) {
                std::vector<uint8_t> file_data(data.begin(), data.end());
                bool success = handle->transfer_manager->handle_file_upload(transfer_id, file_index, file_data);
                response_callback(success, success ? "" : "Failed to write file");
            });
        
        return handle.release();
        
    } catch (const std::exception& e) {
        return nullptr;
    }
}

void warpdeck_destroy(WarpDeckHandle* handle) {
    if (handle) {
        if (handle->started) {
            warpdeck_stop(handle);
        }
        delete handle;
    }
}

int warpdeck_start(WarpDeckHandle* handle, const char* device_name, int desired_port) {
    if (!handle || !device_name) {
        return -1;
    }
    
    try {
        handle->device_name = device_name;
        
        // Set up SSL certificates for API server and client
        std::string cert_file = handle->security_manager->get_certificate_file_path();
        std::string key_file = handle->security_manager->get_private_key_file_path();
        
        handle->api_server->set_ssl_certificate(cert_file, key_file);
        handle->api_client->set_client_certificate(cert_file, key_file);
        
        // Start API server
        DeviceInfo device_info;
        device_info.id = handle->device_id;
        device_info.name = device_name;
        device_info.platform = utils::get_platform_name();
        device_info.protocol_version = "1.0";
        
        LOG_CORE_INFO() << "Starting API server on port " << desired_port;
        if (!handle->api_server->start(desired_port, device_info)) {
            LOG_CORE_ERROR() << "API server failed to start on port " << desired_port;
            return -1;
        }
        
        handle->current_port = handle->api_server->get_port();
        LOG_CORE_INFO() << "API server started successfully on port " << handle->current_port;
        
        // Start discovery manager
        LOG_CORE_DEBUG() << "Getting certificate fingerprint for discovery";
        std::string fingerprint = handle->security_manager->get_certificate_fingerprint();
        LOG_CORE_DEBUG() << "Certificate fingerprint: " << fingerprint.substr(0, 16) << "...";
        
        LOG_CORE_INFO() << "Starting discovery manager for device: " << device_name;
        if (!handle->discovery_manager->start(device_name, handle->device_id, 
                                            device_info.platform, handle->current_port, fingerprint)) {
            LOG_CORE_ERROR() << "Discovery manager failed to start";
            handle->api_server->stop();
            return -1;
        }
        LOG_CORE_INFO() << "Discovery manager started successfully";
        
        handle->started = true;
        return handle->current_port;
        
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return -1;
    }
}

void warpdeck_stop(WarpDeckHandle* handle) {
    if (!handle || !handle->started) {
        return;
    }
    
    try {
        handle->discovery_manager->stop();
        handle->api_server->stop();
        handle->started = false;
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
    }
}

void warpdeck_set_device_name(WarpDeckHandle* handle, const char* new_name) {
    if (!handle || !new_name) {
        return;
    }
    
    try {
        handle->device_name = new_name;
        if (handle->started) {
            handle->discovery_manager->set_device_name(new_name);
        }
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
    }
}

void warpdeck_initiate_transfer(WarpDeckHandle* handle, const char* device_id, const char* files_json) {
    if (!handle || !device_id || !files_json) {
        return;
    }

    try {
        // Parse files_json as a JSON array of file objects
        // Expected format: [{"name": "file.txt", "size": 1024, "path": "/full/path", "relative_path": "folder/file.txt"}, ...]
        nlohmann::json j = nlohmann::json::parse(files_json);

        if (!j.is_array()) {
            safe_call_callback(handle->callbacks.on_error, "files_json must be a JSON array");
            return;
        }

        std::vector<std::string> file_paths;
        std::vector<std::string> relative_paths;

        for (const auto& file_obj : j) {
            // Extract the full path (required for sending)
            if (file_obj.contains("path")) {
                file_paths.push_back(file_obj["path"].get<std::string>());

                // Extract relative_path if present (for folder transfers)
                if (file_obj.contains("relative_path")) {
                    relative_paths.push_back(file_obj["relative_path"].get<std::string>());
                } else if (file_obj.contains("relativePath")) {
                    relative_paths.push_back(file_obj["relativePath"].get<std::string>());
                } else {
                    relative_paths.push_back("");  // No relative path for this file
                }
            }
        }

        if (file_paths.empty()) {
            safe_call_callback(handle->callbacks.on_error, "No valid file paths in JSON");
            return;
        }

        // Get peer info
        auto peers = handle->discovery_manager->get_discovered_peers();
        auto peer_it = peers.find(device_id);
        if (peer_it == peers.end()) {
            safe_call_callback(handle->callbacks.on_error, "Peer not found");
            return;
        }

        const PeerInfo& peer = peer_it->second;

        // Initiate transfer through transfer manager (with relative paths for folder support)
        std::string transfer_id = handle->transfer_manager->initiate_transfer(
            device_id, peer.name, file_paths, relative_paths);

        if (transfer_id.empty()) {
            safe_call_callback(handle->callbacks.on_error, "Failed to initiate transfer");
        }

    } catch (const nlohmann::json::parse_error& e) {
        safe_call_callback(handle->callbacks.on_error, "Invalid JSON format");
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
    }
}

void warpdeck_respond_to_transfer(WarpDeckHandle* handle, const char* transfer_id, bool accept) {
    if (!handle || !transfer_id) {
        return;
    }
    
    try {
        handle->transfer_manager->respond_to_transfer(transfer_id, accept);
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
    }
}

void warpdeck_cancel_transfer(WarpDeckHandle* handle, const char* transfer_id) {
    if (!handle || !transfer_id) {
        return;
    }
    
    try {
        handle->transfer_manager->cancel_transfer(transfer_id);
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
    }
}

const char* warpdeck_get_trusted_devices(WarpDeckHandle* handle) {
    if (!handle) {
        return nullptr;
    }
    
    try {
        auto trusted_peers = handle->security_manager->get_trusted_peers();
        std::string json = utils::trusted_peers_to_json(trusted_peers);
        return copy_string(json);
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return nullptr;
    }
}

void warpdeck_remove_trusted_device(WarpDeckHandle* handle, const char* device_id) {
    if (!handle || !device_id) {
        return;
    }

    try {
        handle->security_manager->remove_trusted_peer(device_id);
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
    }
}

// ============================================================================
// Transfer Queue FFI Functions
// ============================================================================

const char* warpdeck_queue_transfer(WarpDeckHandle* handle, const char* device_id, const char* files_json) {
    if (!handle || !device_id || !files_json) {
        return nullptr;
    }

    try {
        // Parse files_json as a JSON array (same format as initiate_transfer)
        nlohmann::json j = nlohmann::json::parse(files_json);

        if (!j.is_array()) {
            safe_call_callback(handle->callbacks.on_error, "files_json must be a JSON array");
            return nullptr;
        }

        std::vector<std::string> file_paths;
        std::vector<std::string> relative_paths;

        for (const auto& file_obj : j) {
            if (file_obj.contains("path")) {
                file_paths.push_back(file_obj["path"].get<std::string>());

                if (file_obj.contains("relative_path")) {
                    relative_paths.push_back(file_obj["relative_path"].get<std::string>());
                } else if (file_obj.contains("relativePath")) {
                    relative_paths.push_back(file_obj["relativePath"].get<std::string>());
                } else {
                    relative_paths.push_back("");
                }
            }
        }

        if (file_paths.empty()) {
            safe_call_callback(handle->callbacks.on_error, "No valid file paths in JSON");
            return nullptr;
        }

        // Get peer info
        auto peers = handle->discovery_manager->get_discovered_peers();
        auto peer_it = peers.find(device_id);
        if (peer_it == peers.end()) {
            safe_call_callback(handle->callbacks.on_error, "Peer not found");
            return nullptr;
        }

        const PeerInfo& peer = peer_it->second;

        // Queue the transfer
        std::string queue_id = handle->transfer_manager->queue_transfer(
            device_id, peer.name, file_paths, relative_paths);

        if (queue_id.empty()) {
            safe_call_callback(handle->callbacks.on_error, "Failed to queue transfer");
            return nullptr;
        }

        return copy_string(queue_id);

    } catch (const nlohmann::json::parse_error& e) {
        safe_call_callback(handle->callbacks.on_error, "Invalid JSON format");
        return nullptr;
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return nullptr;
    }
}

bool warpdeck_cancel_queued_transfer(WarpDeckHandle* handle, const char* queue_id) {
    if (!handle || !queue_id) {
        return false;
    }

    try {
        return handle->transfer_manager->cancel_queued_transfer(queue_id);
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return false;
    }
}

const char* warpdeck_get_queue_status(WarpDeckHandle* handle) {
    if (!handle) {
        return nullptr;
    }

    try {
        auto queue = handle->transfer_manager->get_queue_status();
        nlohmann::json j = nlohmann::json::array();

        for (const auto& item : queue) {
            nlohmann::json entry;
            entry["queueId"] = item.queue_id;
            entry["peerDeviceId"] = item.peer_device_id;
            entry["peerName"] = item.peer_name;
            entry["status"] = static_cast<int>(item.status);
            entry["transferId"] = item.transfer_id;
            entry["fileCount"] = item.file_count;
            entry["totalBytes"] = item.total_bytes;
            if (!item.error_message.empty()) {
                entry["errorMessage"] = item.error_message;
            }
            j.push_back(entry);
        }

        return copy_string(j.dump());

    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return nullptr;
    }
}

const char* warpdeck_get_discovery_status(WarpDeckHandle* handle) {
    if (!handle || !handle->discovery_manager) {
        return nullptr;
    }

    try {
        const MdnsManager* mdns_manager = handle->discovery_manager->get_mdns_manager();
        if (!mdns_manager) {
            return copy_string("{\"error\": \"MdnsManager not available\"}");
        }

        nlohmann::json status;
        status["publishing"] = mdns_manager->is_publishing();
        status["discovering"] = mdns_manager->is_discovering();
        status["started"] = handle->started;
        status["deviceId"] = handle->device_id;
        status["deviceName"] = handle->device_name;
        status["currentPort"] = handle->current_port;

        return copy_string(status.dump());
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return nullptr;
    }
}

const char* warpdeck_get_discovered_peers(WarpDeckHandle* handle) {
    if (!handle || !handle->discovery_manager) {
        return nullptr;
    }

    try {
        auto peers = handle->discovery_manager->get_discovered_peers();

        nlohmann::json result;
        result["peerCount"] = peers.size();
        result["peers"] = nlohmann::json::array();

        for (const auto& [device_id, peer] : peers) {
            nlohmann::json peer_json;
            peer_json["id"] = peer.id;
            peer_json["name"] = peer.name;
            peer_json["platform"] = peer.platform;
            peer_json["hostAddress"] = peer.host_address;
            peer_json["port"] = peer.port;
            peer_json["fingerprint"] = peer.fingerprint.substr(0, 16) + "...";
            result["peers"].push_back(peer_json);
        }

        return copy_string(result.dump());
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return nullptr;
    }
}

const char* warpdeck_get_mdns_debug_info(WarpDeckHandle* handle) {
    if (!handle || !handle->discovery_manager) {
        return nullptr;
    }
    
    try {
        const MdnsManager* mdns_manager = handle->discovery_manager->get_mdns_manager();
        if (!mdns_manager) {
            return copy_string("MdnsManager not available");
        }
        
        std::string debug_info = mdns_manager->get_debug_info();
        return copy_string(debug_info);
    } catch (const std::exception& e) {
        safe_call_callback(handle->callbacks.on_error, e.what());
        return nullptr;
    }
}

void warpdeck_free_string(const char* str) {
    if (str) {
        delete[] str;
    }
}

} // extern "C"