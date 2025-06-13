#include "cli_application.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <signal.h>
#include <thread>
#include <chrono>
#include <iomanip>

#ifdef __APPLE__
#include <unistd.h>
#include <pwd.h>
#elif __linux__
#include <unistd.h>
#include <pwd.h>
#endif

// Static instance for callbacks
CLIApplication* CLIApplication::instance_ = nullptr;

CLIApplication::CLIApplication() 
    : warpdeck_handle_(nullptr), running_(false) {
    instance_ = this;
    config_dir_ = get_config_dir();
    
    // Ensure config directory exists
    std::filesystem::create_directories(config_dir_);
}

CLIApplication::~CLIApplication() {
    cleanup_warpdeck();
    instance_ = nullptr;
}

int CLIApplication::run(const std::vector<std::string>& args) {
    ParsedCommand cmd = CommandParser::parse(args);
    
    if (!cmd.valid) {
        std::cerr << "Error: " << cmd.error_message << std::endl;
        return 1;
    }
    
    switch (cmd.command) {
        case Command::LISTEN:
            return handle_listen(cmd);
        case Command::LIST:
            return handle_list(cmd);
        case Command::SEND:
            return handle_send(cmd);
        case Command::CONFIG:
            return handle_config(cmd);
        default:
            std::cerr << "Unknown command" << std::endl;
            return 1;
    }
}

int CLIApplication::handle_listen(const ParsedCommand& cmd) {
    std::string device_name = get_default_device_name();
    
    // Override device name if provided
    auto name_it = cmd.options.find("name");
    if (name_it != cmd.options.end()) {
        device_name = name_it->second;
    }
    
    // Set download path if provided
    auto path_it = cmd.options.find("path");
    if (path_it != cmd.options.end()) {
        download_path_ = path_it->second;
    }
    
    if (!initialize_warpdeck(device_name)) {
        return 1;
    }
    
    InteractiveUI::print_discovery_status(true);
    
    running_ = true;
    wait_for_signal();
    
    return 0;
}

int CLIApplication::handle_list(const ParsedCommand& cmd) {
    std::string device_name = get_default_device_name();
    
    // Override device name if provided
    auto name_it = cmd.options.find("name");
    if (name_it != cmd.options.end()) {
        device_name = name_it->second;
    }
    
    if (!initialize_warpdeck(device_name)) {
        return 1;
    }
    
    InteractiveUI::print_discovery_status(false);
    
    // Wait for discovery for a few seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Print discovered peers
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    std::cout << "\nDiscovered Devices:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << std::left << std::setw(25) << "Name" << std::setw(20) << "ID" << "Platform\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    if (discovered_peers_.empty()) {
        std::cout << "No peers found.\n";
    } else {
        for (const auto& [id, peer] : discovered_peers_) {
            std::string display_id = id.length() > 12 ? id.substr(0, 12) + "..." : id;
            std::cout << std::left << std::setw(25) << peer.name 
                     << std::setw(20) << display_id 
                     << peer.platform << "\n";
        }
    }
    
    return 0;
}

int CLIApplication::handle_send(const ParsedCommand& cmd) {
    std::string device_name = get_default_device_name();
    
    // Override device name if provided
    auto name_it = cmd.options.find("name");
    if (name_it != cmd.options.end()) {
        device_name = name_it->second;
    }
    
    if (!initialize_warpdeck(device_name)) {
        return 1;
    }
    
    std::string target_id = cmd.options.at("to");
    
    // First, try to discover the target peer
    std::cout << "🔍 Looking for peer " << target_id.substr(0, 8) << "...\n";
    
    // Wait for discovery
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Check if we found the target peer
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        bool found = false;
        
        for (const auto& [id, peer] : discovered_peers_) {
            if (id == target_id || id.substr(0, target_id.length()) == target_id) {
                found = true;
                std::cout << "✓ Found peer: " << peer.name << "\n";
                target_id = id; // Use full ID
                break;
            }
        }
        
        if (!found) {
            std::cerr << "❌ Peer not found. Available peers:\n";
            for (const auto& [id, peer] : discovered_peers_) {
                std::cerr << "  " << peer.name << " (" << id.substr(0, 8) << "...)\n";
            }
            return 1;
        }
    }
    
    // Verify files exist and build file list JSON
    nlohmann::json files_json = nlohmann::json::array();
    
    for (const std::string& file_path : cmd.arguments) {
        if (!std::filesystem::exists(file_path)) {
            std::cerr << "❌ File not found: " << file_path << "\n";
            return 1;
        }
        
        auto file_size = std::filesystem::file_size(file_path);
        auto file_name = std::filesystem::path(file_path).filename().string();
        
        nlohmann::json file_info;
        file_info["name"] = file_name;
        file_info["size"] = file_size;
        file_info["path"] = file_path;
        
        files_json.push_back(file_info);
        
        std::cout << "📄 " << file_name << " (" << InteractiveUI::format_file_size(file_size) << ")\n";
    }
    
    // Initiate transfer
    std::cout << "🚀 Initiating transfer...\n";
    warpdeck_initiate_transfer(warpdeck_handle_, target_id.c_str(), files_json.dump().c_str());
    
    running_ = true;
    wait_for_signal();
    
    return 0;
}

int CLIApplication::handle_config(const ParsedCommand& cmd) {
    auto name_it = cmd.options.find("set-name");
    if (name_it != cmd.options.end()) {
        const std::string& new_name = name_it->second;
        
        if (save_device_name_to_config(new_name)) {
            std::cout << "✓ Device name set to: " << new_name << "\n";
            return 0;
        } else {
            std::cerr << "❌ Failed to save device name\n";
            return 1;
        }
    }
    
    return 1;
}

bool CLIApplication::initialize_warpdeck(const std::string& device_name, const std::string& config_path) {
    if (warpdeck_handle_) {
        return true; // Already initialized
    }
    
    device_name_ = device_name.empty() ? get_default_device_name() : device_name;
    std::string config_dir = config_path.empty() ? config_dir_ : config_path;
    
    // Setup callbacks
    Callbacks callbacks = {};
    callbacks.on_peer_discovered = on_peer_discovered;
    callbacks.on_peer_lost = on_peer_lost;
    callbacks.on_incoming_transfer_request = on_incoming_transfer_request;
    callbacks.on_transfer_progress_update = on_transfer_progress_update;
    callbacks.on_transfer_completed = on_transfer_completed;
    callbacks.on_error = on_error;
    
    // Create warpdeck instance
    warpdeck_handle_ = warpdeck_create(&callbacks, config_dir.c_str());
    if (!warpdeck_handle_) {
        std::cerr << "❌ Failed to create WarpDeck instance\n";
        return false;
    }
    
    // Start warpdeck
    int port = warpdeck_start(warpdeck_handle_, device_name_.c_str(), 0);
    if (port <= 0) {
        std::cerr << "❌ Failed to start WarpDeck on port " << port << "\n";
        cleanup_warpdeck();
        return false;
    }
    
    std::cout << "✓ WarpDeck started as \"" << device_name_ << "\" on port " << port << "\n";
    return true;
}

void CLIApplication::cleanup_warpdeck() {
    if (warpdeck_handle_) {
        warpdeck_destroy(warpdeck_handle_);
        warpdeck_handle_ = nullptr;
    }
}

// Callback implementations
void CLIApplication::on_peer_discovered(const char* peer_json) {
    if (!instance_) return;
    
    try {
        nlohmann::json j = nlohmann::json::parse(peer_json);
        
        PeerData peer;
        peer.id = j["id"];
        peer.name = j["name"];
        peer.platform = j["platform"];
        
        {
            std::lock_guard<std::mutex> lock(instance_->peers_mutex_);
            instance_->discovered_peers_[peer.id] = peer;
        }
        
        InteractiveUI::print_peer_discovered(peer.name, peer.id, peer.platform);
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing peer data: " << e.what() << std::endl;
    }
}

void CLIApplication::on_peer_lost(const char* device_id) {
    if (!instance_) return;
    
    std::lock_guard<std::mutex> lock(instance_->peers_mutex_);
    auto it = instance_->discovered_peers_.find(device_id);
    if (it != instance_->discovered_peers_.end()) {
        InteractiveUI::print_peer_lost(it->second.name);
        instance_->discovered_peers_.erase(it);
    }
}

void CLIApplication::on_incoming_transfer_request(const char* transfer_request_json) {
    if (!instance_) return;
    
    try {
        nlohmann::json j = nlohmann::json::parse(transfer_request_json);
        
        std::string transfer_id = j["transfer_id"];
        std::string peer_name = j["peer_name"];
        
        std::vector<FileInfo> files;
        for (const auto& file_json : j["files"]) {
            FileInfo file;
            file.name = file_json["name"];
            file.size = file_json["size"];
            file.size_formatted = InteractiveUI::format_file_size(file.size);
            files.push_back(file);
        }
        
        bool accepted = InteractiveUI::prompt_transfer_acceptance(peer_name, files);
        
        // Respond to transfer
        warpdeck_respond_to_transfer(instance_->warpdeck_handle_, transfer_id.c_str(), accepted);
        
        if (accepted) {
            instance_->pending_transfers_[transfer_id] = peer_name;
            std::cout << "✓ Transfer accepted\n";
        } else {
            std::cout << "✗ Transfer declined\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling transfer request: " << e.what() << std::endl;
    }
}

void CLIApplication::on_transfer_progress_update(const char* transfer_id, float progress_percent, uint64_t bytes_transferred) {
    if (!instance_) return;
    
    auto it = instance_->pending_transfers_.find(transfer_id);
    if (it != instance_->pending_transfers_.end()) {
        // For simplicity, show progress for the first file
        InteractiveUI::print_transfer_progress("transfer", progress_percent, bytes_transferred);
    }
}

void CLIApplication::on_transfer_completed(const char* transfer_id, bool success, const char* error_message) {
    if (!instance_) return;
    
    std::string error = error_message ? error_message : "";
    InteractiveUI::print_transfer_completed(transfer_id, success, error);
    
    instance_->pending_transfers_.erase(transfer_id);
    
    if (success) {
        std::cout << "🎉 All files transferred successfully!\n";
    }
}

void CLIApplication::on_error(const char* error_message) {
    if (!instance_) return;
    
    std::cerr << "❌ WarpDeck error: " << error_message << std::endl;
}

// Helper methods
std::string CLIApplication::get_config_dir() {
#ifdef __APPLE__
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : "/tmp";
    }
    return std::string(home) + "/Library/Application Support/WarpDeck";
#elif __linux__
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

std::string CLIApplication::get_default_device_name() {
    // First try to get from config
    std::string saved_name = get_device_name_from_config();
    if (!saved_name.empty()) {
        return saved_name;
    }
    
    // Generate default name
#ifdef __APPLE__
    return "Mac CLI";
#elif __linux__
    return "Linux CLI";
#else
    return "WarpDeck CLI";
#endif
}

std::string CLIApplication::get_device_name_from_config() {
    std::string config_file = config_dir_ + "/config.json";
    
    if (!std::filesystem::exists(config_file)) {
        return "";
    }
    
    try {
        std::ifstream file(config_file);
        nlohmann::json config;
        file >> config;
        
        return config.value("device_name", "");
    } catch (const std::exception&) {
        return "";
    }
}

bool CLIApplication::save_device_name_to_config(const std::string& name) {
    std::string config_file = config_dir_ + "/config.json";
    
    try {
        nlohmann::json config;
        
        // Load existing config if it exists
        if (std::filesystem::exists(config_file)) {
            std::ifstream file(config_file);
            file >> config;
        }
        
        config["device_name"] = name;
        
        std::ofstream file(config_file);
        file << config.dump(2);
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void CLIApplication::wait_for_signal() {
    // Set up signal handling
    signal(SIGINT, [](int signal) {
        if (instance_) {
            instance_->running_ = false;
            std::cout << "\n🛑 Shutting down...\n";
        }
    });
    
    // Wait for signal
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}