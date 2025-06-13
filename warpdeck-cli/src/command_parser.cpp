#include "command_parser.h"
#include <algorithm>

ParsedCommand CommandParser::parse(const std::vector<std::string>& args) {
    ParsedCommand result;
    result.valid = false;
    
    if (args.empty()) {
        result.error_message = "No command specified";
        return result;
    }
    
    // Parse command
    result.command = parse_command(args[0]);
    if (result.command == Command::UNKNOWN) {
        result.error_message = "Unknown command: " + args[0];
        return result;
    }
    
    // Parse options and arguments
    if (!parse_options(args, 1, result)) {
        return result;
    }
    
    // Validate command-specific requirements
    switch (result.command) {
        case Command::SEND:
            if (result.options.find("to") == result.options.end()) {
                result.error_message = "send command requires --to option";
                return result;
            }
            if (result.arguments.empty()) {
                result.error_message = "send command requires at least one file";
                return result;
            }
            break;
            
        case Command::CONFIG:
            if (result.options.find("set-name") == result.options.end()) {
                result.error_message = "config command requires --set-name option";
                return result;
            }
            break;
            
        case Command::LISTEN:
        case Command::LIST:
            // No specific requirements
            break;
            
        case Command::UNKNOWN:
            // Already handled above
            break;
    }
    
    result.valid = true;
    return result;
}

Command CommandParser::parse_command(const std::string& cmd) {
    if (cmd == "listen") return Command::LISTEN;
    if (cmd == "list") return Command::LIST;
    if (cmd == "send") return Command::SEND;
    if (cmd == "config") return Command::CONFIG;
    return Command::UNKNOWN;
}

bool CommandParser::parse_options(const std::vector<std::string>& args, size_t start_index, ParsedCommand& result) {
    for (size_t i = start_index; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg.length() >= 2 && arg.substr(0, 2) == "--") {
            std::string option = arg.substr(2);
            
            // Options that require a value
            if (option == "to" || option == "name" || option == "path" || option == "set-name") {
                if (i + 1 >= args.size()) {
                    result.error_message = "Option --" + option + " requires a value";
                    return false;
                }
                result.options[option] = args[++i];
            } else {
                result.error_message = "Unknown option: --" + option;
                return false;
            }
        } else {
            // Regular argument (file paths for send command)
            result.arguments.push_back(arg);
        }
    }
    
    return true;
}