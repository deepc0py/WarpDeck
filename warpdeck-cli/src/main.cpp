#include "cli_application.h"
#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>

bool validate_system_dependencies() {
    std::vector<std::string> required_libs = {
        "libssl.so.1.1",
        "libssl.so.3",      // Ubuntu 22.04+ fallback
        "libcrypto.so.1.1", 
        "libcrypto.so.3",   // Ubuntu 22.04+ fallback
        "libavahi-client.so.3",
        "libavahi-common.so.3"
    };
    
    std::vector<std::string> missing_libs;
    bool ssl_found = false;
    bool crypto_found = false;
    bool avahi_client_found = false;
    bool avahi_common_found = false;
    
    for (const auto& lib : required_libs) {
        void* handle = dlopen(lib.c_str(), RTLD_LAZY | RTLD_NOLOAD);
        if (handle) {
            dlclose(handle);
            // Track which critical libraries we found
            if (lib.find("libssl") != std::string::npos) ssl_found = true;
            if (lib.find("libcrypto") != std::string::npos) crypto_found = true;
            if (lib.find("libavahi-client") != std::string::npos) avahi_client_found = true;
            if (lib.find("libavahi-common") != std::string::npos) avahi_common_found = true;
        }
    }
    
    // Check if we have at least one version of each critical library
    if (!ssl_found) missing_libs.push_back("libssl (any version)");
    if (!crypto_found) missing_libs.push_back("libcrypto (any version)");
    if (!avahi_client_found) missing_libs.push_back("libavahi-client.so.3");
    if (!avahi_common_found) missing_libs.push_back("libavahi-common.so.3");
    
    if (!missing_libs.empty()) {
        std::cerr << "❌ Missing required system libraries:\n";
        for (const auto& lib : missing_libs) {
            std::cerr << "   - " << lib << "\n";
        }
        std::cerr << "\n💡 To install missing dependencies:\n";
        std::cerr << "   Ubuntu/Debian: sudo apt install libssl-dev libavahi-client-dev\n";
        std::cerr << "   RHEL/CentOS:   sudo yum install openssl-devel avahi-devel\n";
        std::cerr << "   Arch Linux:    sudo pacman -S openssl avahi\n";
        return false;
    }
    
    return true;
}

void print_usage(const char* program_name) {
    std::cout << "WarpDeck CLI - Peer-to-peer file sharing\n";
    std::cout << "Usage: " << program_name << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  listen                        Start listening for incoming transfers\n";
    std::cout << "  list                          List discovered peers\n";
    std::cout << "  send --to <id> <files...>     Send files to a peer\n";
    std::cout << "  config --set-name <name>      Set device name\n\n";
    std::cout << "Options:\n";
    std::cout << "  --name <name>                 Override device name for this session\n";
    std::cout << "  --path <path>                 Set download directory for this session\n";
    std::cout << "  --help                        Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " listen\n";
    std::cout << "  " << program_name << " list\n";
    std::cout << "  " << program_name << " send --to abc123 file1.txt file2.jpg\n";
    std::cout << "  " << program_name << " config --set-name \"My Device\"\n";
}

int main(int argc, char* argv[]) {
    // Validate system dependencies before doing anything else
    if (!validate_system_dependencies()) {
        return 1;
    }

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Convert arguments to string vector
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    // Check for help
    if (args[0] == "--help" || args[0] == "-h") {
        print_usage(argv[0]);
        return 0;
    }

    try {
        CLIApplication app;
        return app.run(args);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}