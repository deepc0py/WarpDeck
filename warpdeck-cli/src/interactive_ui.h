#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

struct FileInfo {
    std::string name;
    uint64_t size;
    std::string size_formatted;
};

class InteractiveUI {
public:
    static bool prompt_transfer_acceptance(const std::string& peer_name, 
                                         const std::vector<FileInfo>& files);
    
    static void print_discovery_status(bool listening);
    static void print_peer_discovered(const std::string& name, const std::string& id, const std::string& platform);
    static void print_peer_lost(const std::string& name);
    static void print_transfer_progress(const std::string& filename, float progress, uint64_t bytes_per_second);
    static void print_transfer_completed(const std::string& filename, bool success, const std::string& error = "");
    static void print_file_saved(const std::string& filename, const std::string& path);
    
    static std::string format_file_size(uint64_t bytes);
    static std::string format_transfer_speed(uint64_t bytes_per_second);

private:
    static std::string get_user_input(const std::string& prompt);
    static bool parse_yes_no(const std::string& input);
};