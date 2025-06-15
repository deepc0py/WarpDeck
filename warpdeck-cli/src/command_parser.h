#pragma once

#include <string>
#include <vector>
#include <map>

enum class Command {
    LISTEN,
    LIST,
    SEND,
    CONFIG,
    DEBUG,
    UNKNOWN
};

struct ParsedCommand {
    Command command;
    std::map<std::string, std::string> options;
    std::vector<std::string> arguments;
    std::string error_message;
    bool valid;
};

class CommandParser {
public:
    static ParsedCommand parse(const std::vector<std::string>& args);

private:
    static Command parse_command(const std::string& cmd);
    static bool parse_options(const std::vector<std::string>& args, size_t start_index, ParsedCommand& result);
};