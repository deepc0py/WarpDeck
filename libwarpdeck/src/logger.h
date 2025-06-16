#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <mutex>
#include <functional>

namespace warpdeck {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

class Logger {
public:
    using LogCallback = std::function<void(LogLevel level, const std::string& component, const std::string& message)>;
    
    static Logger& instance();
    
    void set_log_level(LogLevel level);
    void set_log_callback(LogCallback callback);
    void enable_console_output(bool enable);
    
    void log(LogLevel level, const std::string& component, const std::string& message);
    
    // Convenience methods
    void trace(const std::string& component, const std::string& message);
    void debug(const std::string& component, const std::string& message);
    void info(const std::string& component, const std::string& message);
    void warn(const std::string& component, const std::string& message);
    void error(const std::string& component, const std::string& message);
    void fatal(const std::string& component, const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string format_message(LogLevel level, const std::string& component, const std::string& message);
    std::string log_level_to_string(LogLevel level);
    std::string get_timestamp();
    
    std::mutex mutex_;
    LogLevel min_level_ = LogLevel::INFO;
    LogCallback log_callback_;
    bool console_output_enabled_ = true;
};

// Stream-based logging helper
class LogStream {
public:
    LogStream(LogLevel level, const std::string& component);
    ~LogStream();
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    LogLevel level_;
    std::string component_;
    std::ostringstream stream_;
};

// Macros for convenient logging
#define WARPDECK_LOG_TRACE(component) warpdeck::LogStream(warpdeck::LogLevel::TRACE, component)
#define WARPDECK_LOG_DEBUG(component) warpdeck::LogStream(warpdeck::LogLevel::DEBUG, component)
#define WARPDECK_LOG_INFO(component) warpdeck::LogStream(warpdeck::LogLevel::INFO, component)
#define WARPDECK_LOG_WARN(component) warpdeck::LogStream(warpdeck::LogLevel::WARN, component)
#define WARPDECK_LOG_ERROR(component) warpdeck::LogStream(warpdeck::LogLevel::ERROR, component)
#define WARPDECK_LOG_FATAL(component) warpdeck::LogStream(warpdeck::LogLevel::FATAL, component)

// Component-specific macros
#define LOG_DISCOVERY_TRACE() WARPDECK_LOG_TRACE("Discovery")
#define LOG_DISCOVERY_DEBUG() WARPDECK_LOG_DEBUG("Discovery")
#define LOG_DISCOVERY_INFO() WARPDECK_LOG_INFO("Discovery")
#define LOG_DISCOVERY_WARN() WARPDECK_LOG_WARN("Discovery")
#define LOG_DISCOVERY_ERROR() WARPDECK_LOG_ERROR("Discovery")

#define LOG_SECURITY_TRACE() WARPDECK_LOG_TRACE("Security")
#define LOG_SECURITY_DEBUG() WARPDECK_LOG_DEBUG("Security")
#define LOG_SECURITY_INFO() WARPDECK_LOG_INFO("Security")
#define LOG_SECURITY_WARN() WARPDECK_LOG_WARN("Security")
#define LOG_SECURITY_ERROR() WARPDECK_LOG_ERROR("Security")

#define LOG_TRANSFER_TRACE() WARPDECK_LOG_TRACE("Transfer")
#define LOG_TRANSFER_DEBUG() WARPDECK_LOG_DEBUG("Transfer")
#define LOG_TRANSFER_INFO() WARPDECK_LOG_INFO("Transfer")
#define LOG_TRANSFER_WARN() WARPDECK_LOG_WARN("Transfer")
#define LOG_TRANSFER_ERROR() WARPDECK_LOG_ERROR("Transfer")

#define LOG_API_TRACE() WARPDECK_LOG_TRACE("API")
#define LOG_API_DEBUG() WARPDECK_LOG_DEBUG("API")
#define LOG_API_INFO() WARPDECK_LOG_INFO("API")
#define LOG_API_WARN() WARPDECK_LOG_WARN("API")
#define LOG_API_ERROR() WARPDECK_LOG_ERROR("API")

#define LOG_CORE_TRACE() WARPDECK_LOG_TRACE("Core")
#define LOG_CORE_DEBUG() WARPDECK_LOG_DEBUG("Core")
#define LOG_CORE_INFO() WARPDECK_LOG_INFO("Core")
#define LOG_CORE_WARN() WARPDECK_LOG_WARN("Core")
#define LOG_CORE_ERROR() WARPDECK_LOG_ERROR("Core")

} // namespace warpdeck