#include "logger.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

namespace warpdeck {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_log_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::set_log_callback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_callback_ = callback;
}

void Logger::enable_console_output(bool enable) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_output_enabled_ = enable;
}

void Logger::log(LogLevel level, const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if this message should be logged based on minimum level
    if (level < min_level_) {
        return;
    }
    
    std::string formatted_message = format_message(level, component, message);
    
    // Call user-provided callback if set
    if (log_callback_) {
        try {
            log_callback_(level, component, message);
        } catch (...) {
            // Ignore callback exceptions to prevent logging from crashing the application
        }
    }
    
    // Console output
    if (console_output_enabled_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }
}

void Logger::trace(const std::string& component, const std::string& message) {
    log(LogLevel::TRACE, component, message);
}

void Logger::debug(const std::string& component, const std::string& message) {
    log(LogLevel::DEBUG, component, message);
}

void Logger::info(const std::string& component, const std::string& message) {
    log(LogLevel::INFO, component, message);
}

void Logger::warn(const std::string& component, const std::string& message) {
    log(LogLevel::WARN, component, message);
}

void Logger::error(const std::string& component, const std::string& message) {
    log(LogLevel::ERROR, component, message);
}

void Logger::fatal(const std::string& component, const std::string& message) {
    log(LogLevel::FATAL, component, message);
}

std::string Logger::format_message(LogLevel level, const std::string& component, const std::string& message) {
    std::ostringstream oss;
    oss << "[" << get_timestamp() << "] [" << log_level_to_string(level) << "] [" << component << "] " << message;
    return oss.str();
}

std::string Logger::log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// LogStream implementation
LogStream::LogStream(LogLevel level, const std::string& component) 
    : level_(level), component_(component) {
}

LogStream::~LogStream() {
    Logger::instance().log(level_, component_, stream_.str());
}

} // namespace warpdeck