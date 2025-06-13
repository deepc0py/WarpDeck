#include "cli_application.h"
#include <iostream>
#include <vector>
#include <string>

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