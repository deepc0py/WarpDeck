#include "interactive_ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdint>

bool InteractiveUI::prompt_transfer_acceptance(const std::string& peer_name, 
                                              const std::vector<FileInfo>& files) {
    std::cout << "\n━━━ Incoming Transfer Request ━━━\n";
    std::cout << "From: " << peer_name << "\n";
    std::cout << "Files:\n";
    
    for (const auto& file : files) {
        std::cout << "  • " << file.name << " (" << file.size_formatted << ")\n";
    }
    
    std::cout << "\nAccept transfer? (y/N): ";
    std::string input = get_user_input("");
    
    return parse_yes_no(input);
}

void InteractiveUI::print_discovery_status(bool listening) {
    if (listening) {
        std::cout << "🔍 WarpDeck listening for peers and transfers...\n";
        std::cout << "   Press Ctrl+C to stop\n\n";
    } else {
        std::cout << "🔍 Scanning for peers...\n\n";
    }
}

void InteractiveUI::print_peer_discovered(const std::string& name, const std::string& id, const std::string& platform) {
    std::string icon = (platform == "macos") ? "🖥️" : "🎮";
    std::cout << "✓ Found peer: " << icon << " " << name << " (ID: " << id.substr(0, 8) << "...)\n";
}

void InteractiveUI::print_peer_lost(const std::string& name) {
    std::cout << "✗ Lost peer: " << name << "\n";
}

void InteractiveUI::print_transfer_progress(const std::string& filename, float progress, uint64_t bytes_per_second) {
    const int bar_width = 40;
    int filled = static_cast<int>(progress * bar_width / 100.0f);
    
    std::cout << "\r📤 " << filename << " [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            std::cout << "═";
        } else if (i == filled) {
            std::cout << ">";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << progress << "% ";
    std::cout << format_transfer_speed(bytes_per_second);
    std::cout.flush();
}

void InteractiveUI::print_transfer_completed(const std::string& filename, bool success, const std::string& error) {
    std::cout << "\n";
    if (success) {
        std::cout << "✅ Transfer completed: " << filename << "\n";
    } else {
        std::cout << "❌ Transfer failed: " << filename;
        if (!error.empty()) {
            std::cout << " (" << error << ")";
        }
        std::cout << "\n";
    }
}

void InteractiveUI::print_file_saved(const std::string& filename, const std::string& path) {
    std::cout << "💾 Saved: " << path << "\n";
}

std::string InteractiveUI::format_file_size(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    if (unit_index == 0) {
        oss << static_cast<uint64_t>(size) << " " << units[unit_index];
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    }
    
    return oss.str();
}

std::string InteractiveUI::format_transfer_speed(uint64_t bytes_per_second) {
    if (bytes_per_second == 0) {
        return "0 B/s";
    }
    
    return format_file_size(bytes_per_second) + "/s";
}

std::string InteractiveUI::get_user_input(const std::string& prompt) {
    if (!prompt.empty()) {
        std::cout << prompt;
    }
    
    std::string input;
    std::getline(std::cin, input);
    return input;
}

bool InteractiveUI::parse_yes_no(const std::string& input) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    return (lower_input == "y" || lower_input == "yes");
}