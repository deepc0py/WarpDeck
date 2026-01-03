#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <vector>
#include <map>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

#include "third_party/mjansson_mdns/mdns.h"

// Simple mDNS test to verify multicast discovery
class MdnsTest {
public:
    MdnsTest() : running_(false) {}
    
    bool init() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            return false;
        }
#endif
        
        // Create IPv4 socket
        sock_ = mdns_socket_open_ipv4(nullptr);
        if (sock_ < 0) {
            std::cerr << "Failed to open IPv4 mDNS socket" << std::endl;
            return false;
        }
        
        std::cout << "✅ Opened mDNS socket: " << sock_ << std::endl;
        show_network_info();
        return true;
    }
    
    void show_network_info() {
        std::cout << "\n📡 Network Configuration:" << std::endl;
        
#ifdef _WIN32
        // Windows implementation
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            std::cout << "  Hostname: " << hostname << std::endl;
        }
#else
        // Unix implementation
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            std::cout << "  Hostname: " << hostname << std::endl;
        }
        
        struct ifaddrs* ifaddrs_ptr;
        if (getifaddrs(&ifaddrs_ptr) == 0) {
            std::cout << "  Network Interfaces:" << std::endl;
            for (struct ifaddrs* ifa = ifaddrs_ptr; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr) continue;
                
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    char addr_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr,
                             addr_str, INET_ADDRSTRLEN);
                    
                    // Skip loopback
                    if (strncmp(addr_str, "127.", 4) != 0) {
                        std::cout << "    " << ifa->ifa_name << ": " << addr_str << std::endl;
                    }
                }
            }
            freeifaddrs(ifaddrs_ptr);
        }
#endif
    }
    
    void send_query() {
        const char* service_name = "_warpdeck._tcp.local.";
        char buffer[256];
        
        int query_id = mdns_query_send(sock_, MDNS_RECORDTYPE_PTR, 
                                      service_name, strlen(service_name),
                                      buffer, sizeof(buffer), 0);
        
        if (query_id > 0) {
            std::cout << "📤 Sent mDNS query for " << service_name 
                     << " (ID: " << query_id << ")" << std::endl;
        } else {
            std::cerr << "❌ Failed to send mDNS query" << std::endl;
        }
    }
    
    void announce_service(const std::string& device_id, int port) {
        std::cout << "📢 Announcing service: " << device_id << " on port " << port << std::endl;
        
        // Send a simple mDNS announcement
        char buffer[2048];
        std::string service_instance = device_id + "._warpdeck._tcp.local.";
        std::string service_type = "_warpdeck._tcp.local.";
        
        mdns_record_t record;
        record.name.str = service_type.c_str();
        record.name.length = service_type.length();
        record.type = MDNS_RECORDTYPE_PTR;
        record.data.ptr.name.str = service_instance.c_str();
        record.data.ptr.name.length = service_instance.length();
        
        int result = mdns_announce_multicast(sock_, buffer, sizeof(buffer),
                                            record, nullptr, 0, nullptr, 0);
        
        if (result > 0) {
            std::cout << "✅ Announcement sent (" << result << " bytes)" << std::endl;
        } else {
            std::cerr << "❌ Failed to send announcement" << std::endl;
        }
    }
    
    void listen_for_responses(int timeout_seconds) {
        std::cout << "\n👂 Listening for mDNS responses for " << timeout_seconds << " seconds..." << std::endl;
        
        auto start = std::chrono::steady_clock::now();
        while (true) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
            if (elapsed.count() >= timeout_seconds) break;
            
            fd_set readfs;
            FD_ZERO(&readfs);
            FD_SET(sock_, &readfs);
            
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms
            
            if (select(sock_ + 1, &readfs, nullptr, nullptr, &timeout) > 0) {
                if (FD_ISSET(sock_, &readfs)) {
                    char buffer[2048];
                    struct sockaddr_storage from_addr;
                    socklen_t from_len = sizeof(from_addr);
                    
                    ssize_t bytes = recvfrom(sock_, buffer, sizeof(buffer), 0,
                                           (struct sockaddr*)&from_addr, &from_len);
                    
                    if (bytes > 0) {
                        process_packet(buffer, bytes, (struct sockaddr*)&from_addr);
                    }
                }
            }
        }
    }
    
    void process_packet(const char* buffer, size_t size, struct sockaddr* from) {
        // Get sender address
        char addr_str[INET6_ADDRSTRLEN];
        if (from->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)from;
            inet_ntop(AF_INET, &addr_in->sin_addr, addr_str, INET_ADDRSTRLEN);
        } else if (from->sa_family == AF_INET6) {
            struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)from;
            inet_ntop(AF_INET6, &addr_in6->sin6_addr, addr_str, INET6_ADDRSTRLEN);
        } else {
            strcpy(addr_str, "unknown");
        }
        
        // Parse mDNS header
        if (size < 12) return;
        
        const uint16_t* data = (const uint16_t*)buffer;
        uint16_t query_id = ntohs(data[0]);
        uint16_t flags = ntohs(data[1]);
        uint16_t questions = ntohs(data[2]);
        uint16_t answer_rrs = ntohs(data[3]);
        
        if (questions > 0 || answer_rrs > 0) {
            std::cout << "📨 Received mDNS packet from " << addr_str 
                     << " (Q:" << questions << " A:" << answer_rrs << ")" << std::endl;
            
            // Look for WarpDeck services
            size_t offset = 12;
            char name_buffer[256];
            
            // Skip questions
            for (uint16_t i = 0; i < questions && offset < size; ++i) {
                mdns_string_skip(buffer, size, &offset);
                offset += 4; // Skip type and class
            }
            
            // Process answers
            for (uint16_t i = 0; i < answer_rrs && offset < size; ++i) {
                size_t name_offset = offset;
                mdns_string_skip(buffer, size, &offset);
                
                if (offset + 10 > size) break;
                
                mdns_string_t name = mdns_string_extract(buffer, size, &name_offset,
                                                        name_buffer, sizeof(name_buffer));
                std::string record_name(name.str, name.length);
                
                if (record_name.find("_warpdeck") != std::string::npos) {
                    std::cout << "  🎯 Found WarpDeck service: " << record_name << std::endl;
                }
                
                const uint16_t* record_data = (const uint16_t*)((const char*)buffer + offset);
                uint16_t record_length = ntohs(record_data[4]);
                offset += 10 + record_length;
            }
        }
    }
    
    void cleanup() {
        if (sock_ >= 0) {
            mdns_socket_close(sock_);
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
private:
    int sock_ = -1;
    bool running_;
};

int main(int argc, char* argv[]) {
    std::cout << "🔍 mDNS Cross-Platform Discovery Test\n" << std::endl;
    std::cout << "Platform: ";
#ifdef _WIN32
    std::cout << "Windows";
#elif __APPLE__
    std::cout << "macOS";
#elif __linux__
    std::cout << "Linux";
#else
    std::cout << "Unknown";
#endif
    std::cout << std::endl;
    
    MdnsTest test;
    if (!test.init()) {
        return 1;
    }
    
    // Parse command line
    bool announce = false;
    std::string device_id = "test-device";
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--announce") == 0) {
            announce = true;
        } else if (strcmp(argv[i], "--id") == 0 && i + 1 < argc) {
            device_id = argv[++i];
        }
    }
    
    if (announce) {
        std::cout << "\n🚀 Running in ANNOUNCE mode\n" << std::endl;
        
        // Announce our service periodically
        for (int i = 0; i < 10; ++i) {
            test.announce_service(device_id, 54321);
            test.listen_for_responses(3);
        }
    } else {
        std::cout << "\n🔎 Running in DISCOVERY mode\n" << std::endl;
        std::cout << "Tip: Run with --announce --id <device-id> on another machine\n" << std::endl;
        
        // Send queries and listen for responses
        for (int i = 0; i < 10; ++i) {
            test.send_query();
            test.listen_for_responses(3);
        }
    }
    
    test.cleanup();
    std::cout << "\n✅ Test completed" << std::endl;
    return 0;
}