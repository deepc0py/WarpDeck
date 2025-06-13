#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace warpdeck {

// Forward declarations
struct PeerInfo;
struct DeviceInfo;
struct TransferRequest;
struct FileMetadata;
struct TrustedPeer;

namespace utils {

// UUID generation
std::string generate_uuid();

// JSON utilities
std::string peer_info_to_json(const PeerInfo& peer);
std::string device_info_to_json(const DeviceInfo& device);
std::string transfer_request_to_json(const TransferRequest& request);
std::string file_metadata_to_json(const FileMetadata& file);
std::string trusted_peers_to_json(const std::map<std::string, TrustedPeer>& peers);

// JSON parsing
bool parse_transfer_request(const std::string& json, TransferRequest& request);
bool parse_file_metadata(const nlohmann::json& json, FileMetadata& file);

// File utilities
bool file_exists(const std::string& path);
bool create_directory(const std::string& path);
std::string get_parent_directory(const std::string& path);
std::string get_filename(const std::string& path);
uint64_t get_file_size(const std::string& path);
std::string calculate_file_hash(const std::string& path);

// Platform utilities
std::string get_platform_name();
std::string get_default_config_dir();
std::string get_default_download_dir();

// Time utilities
std::string get_iso8601_timestamp();
std::string get_expiry_timestamp(int minutes_from_now = 30);

} // namespace utils
} // namespace warpdeck