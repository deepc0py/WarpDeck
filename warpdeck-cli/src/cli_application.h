#pragma once

#include "command_parser.h"
#include "interactive_ui.h"
#include "warpdeck.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>
#include <mutex>

struct PeerData {
    std::string id;
    std::string name;
    std::string platform;
};

class CLIApplication {
public:
    CLIApplication();
    ~CLIApplication();

    int run(const std::vector<std::string>& args);

private:
    // Command handlers
    int handle_listen(const ParsedCommand& cmd);
    int handle_list(const ParsedCommand& cmd);
    int handle_send(const ParsedCommand& cmd);
    int handle_config(const ParsedCommand& cmd);
    int handle_debug(const ParsedCommand& cmd);

    // libwarpdeck callback handlers
    static void on_peer_discovered(const char* peer_json);
    static void on_peer_lost(const char* device_id);
    static void on_incoming_transfer_request(const char* transfer_request_json);
    static void on_transfer_progress_update(const char* transfer_id, float progress_percent, uint64_t bytes_transferred);
    static void on_transfer_completed(const char* transfer_id, bool success, const char* error_message);
    static void on_error(const char* error_message);

    // Helper methods
    bool initialize_warpdeck(const std::string& device_name = "", const std::string& config_path = "");
    void cleanup_warpdeck();
    std::string get_config_dir();
    std::string get_default_device_name();
    std::string get_device_name_from_config();
    bool save_device_name_to_config(const std::string& name);
    void wait_for_signal();
    
    // Static instance for callbacks
    static CLIApplication* instance_;
    
    // Member variables
    WarpDeckHandle* warpdeck_handle_;
    std::atomic<bool> running_;
    std::mutex peers_mutex_;
    std::map<std::string, PeerData> discovered_peers_;
    std::map<std::string, std::string> pending_transfers_;
    
    // Configuration
    std::string config_dir_;
    std::string device_name_;
    std::string download_path_;
};