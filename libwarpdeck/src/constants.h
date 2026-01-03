#pragma once

#include <cstdint>

namespace warpdeck {
namespace constants {

// Network ports
// Note: Using WARPDECK_ prefix to avoid collision with mdns.h macros
constexpr int WARPDECK_MDNS_PORT = 5353;
constexpr int WARPDECK_DEFAULT_API_PORT = 54321;
constexpr int WARPDECK_MAX_PORT = 65535;

// mDNS TTL values (in seconds)
constexpr uint32_t TTL_PTR_RECORD = 4500;    // 75 minutes for PTR/TXT records
constexpr uint32_t TTL_SRV_RECORD = 120;     // 2 minutes for SRV/A records
constexpr uint32_t TTL_DEFAULT = 300;        // 5 minutes default

// Service discovery
constexpr const char* SERVICE_TYPE = "_warpdeck._tcp.local.";
constexpr const char* SERVICE_TYPE_SHORT = "_warpdeck._tcp";

// Protocol
constexpr const char* PROTOCOL_VERSION = "1.0";

// Timing intervals (in seconds)
constexpr int DISCOVERY_QUERY_INTERVAL = 3;
constexpr int PEER_TIMEOUT_CHECK_INTERVAL = 10;
constexpr int ANNOUNCEMENT_INTERVAL = 30;

// Buffer sizes
constexpr size_t WARPDECK_MDNS_BUFFER_SIZE = 2048;
constexpr size_t WARPDECK_HOSTNAME_BUFFER_SIZE = 256;

} // namespace constants
} // namespace warpdeck
