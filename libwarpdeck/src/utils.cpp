#include "utils.h"
#include "discovery_manager.h"
#include "api_server.h"
#include "security_manager.h"
#include "transfer_manager.h"
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <openssl/sha.h>

#ifdef WARPDECK_PLATFORM_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <pwd.h>
#elif defined(WARPDECK_PLATFORM_LINUX)
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#endif

namespace warpdeck {
namespace utils {

std::string generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

std::string peer_info_to_json(const PeerInfo& peer) {
    nlohmann::json j;
    j["id"] = peer.id;
    j["name"] = peer.name;
    j["platform"] = peer.platform;
    j["port"] = peer.port;
    j["fingerprint"] = peer.fingerprint;
    j["host_address"] = peer.host_address;
    return j.dump();
}

std::string device_info_to_json(const DeviceInfo& device) {
    nlohmann::json j;
    j["id"] = device.id;
    j["name"] = device.name;
    j["platform"] = device.platform;
    j["protocol_version"] = device.protocol_version;
    return j.dump();
}

std::string transfer_request_to_json(const TransferRequest& request) {
    nlohmann::json j;
    j["files"] = nlohmann::json::array();
    
    for (const auto& file : request.files) {
        nlohmann::json file_json;
        file_json["name"] = file.name;
        file_json["size"] = file.size;
        if (!file.hash.empty()) {
            file_json["hash"] = file.hash;
        }
        j["files"].push_back(file_json);
    }
    
    return j.dump();
}

std::string file_metadata_to_json(const FileMetadata& file) {
    nlohmann::json j;
    j["name"] = file.name;
    j["size"] = file.size;
    if (!file.hash.empty()) {
        j["hash"] = file.hash;
    }
    return j.dump();
}

std::string trusted_peers_to_json(const std::map<std::string, TrustedPeer>& peers) {
    nlohmann::json j = nlohmann::json::array();
    
    for (const auto& [device_id, peer] : peers) {
        nlohmann::json peer_json;
        peer_json["device_id"] = peer.device_id;
        peer_json["fingerprint"] = peer.fingerprint;
        peer_json["name"] = peer.name;
        j.push_back(peer_json);
    }
    
    return j.dump();
}

bool parse_transfer_request(const std::string& json, TransferRequest& request) {
    try {
        nlohmann::json j = nlohmann::json::parse(json);
        
        if (!j.contains("files") || !j["files"].is_array()) {
            return false;
        }
        
        request.files.clear();
        for (const auto& file_json : j["files"]) {
            FileMetadata file;
            if (!parse_file_metadata(file_json, file)) {
                return false;
            }
            request.files.push_back(file);
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_file_metadata(const nlohmann::json& json, FileMetadata& file) {
    try {
        if (!json.contains("name") || !json.contains("size")) {
            return false;
        }
        
        file.name = json["name"];
        file.size = json["size"];
        
        if (json.contains("hash")) {
            file.hash = json["hash"];
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool create_directory(const std::string& path) {
    try {
        // Check if directory already exists
        if (std::filesystem::exists(path)) {
            return std::filesystem::is_directory(path);
        }
        // Create the directory and return success
        return std::filesystem::create_directories(path);
    } catch (const std::exception&) {
        return false;
    }
}

std::string get_parent_directory(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

std::string get_filename(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

uint64_t get_file_size(const std::string& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (const std::exception&) {
        return 0;
    }
}

std::string calculate_file_hash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::string get_platform_name() {
#ifdef WARPDECK_PLATFORM_MACOS
    return "macos";
#elif defined(WARPDECK_PLATFORM_LINUX)
    return "steamdeck";
#else
    return "unknown";
#endif
}

std::string get_default_config_dir() {
#ifdef WARPDECK_PLATFORM_MACOS
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/tmp";
    }
    return std::string(home) + "/Library/Application Support/WarpDeck";
#elif defined(WARPDECK_PLATFORM_LINUX)
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/tmp";
    }
    return std::string(home) + "/.config/warpdeck";
#else
    return "/tmp/warpdeck";
#endif
}

std::string get_default_download_dir() {
#ifdef WARPDECK_PLATFORM_MACOS
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/tmp";
    }
    return std::string(home) + "/Downloads";
#elif defined(WARPDECK_PLATFORM_LINUX)
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/tmp";
    }
    return std::string(home) + "/Downloads";
#else
    return "/tmp";
#endif
}

std::string get_iso8601_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return ss.str();
}

std::string get_expiry_timestamp(int minutes_from_now) {
    auto now = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::minutes(minutes_from_now);
    auto time_t = std::chrono::system_clock::to_time_t(expiry);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    
    return ss.str();
}

} // namespace utils
} // namespace warpdeck