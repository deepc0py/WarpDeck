#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <signal.h>

// Direct test of mDNS library functionality
extern "C" {
#include "third_party/mjansson_mdns/mdns.h"
}

static volatile bool g_running = true;

void signal_handler(int sig) {
    g_running = false;
}

int query_callback(int sock, const struct sockaddr* from, size_t addrlen,
                   mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
                   uint16_t rclass, uint32_t ttl, const void* data, size_t size,
                   size_t name_offset, size_t name_length, size_t record_offset,
                   size_t record_length, void* user_data) {
    
    char name_buffer[256];
    mdns_string_t name = mdns_string_extract(data, size, &name_offset,
                                           name_buffer, sizeof(name_buffer));
    
    std::string record_name(name.str, name.length);
    
    if (record_name.find("_warpdeck._tcp") != std::string::npos) {
        std::cout << "Found _warpdeck service!" << std::endl;
        std::cout << "  Entry type: " << entry << ", Record type: " << rtype << std::endl;
        std::cout << "  Name: " << record_name << std::endl;
        
        if (rtype == MDNS_RECORDTYPE_PTR && (entry == MDNS_ENTRYTYPE_ANSWER || 
                                             entry == MDNS_ENTRYTYPE_ADDITIONAL)) {
            char ptr_buffer[256];
            mdns_string_t ptr = mdns_string_extract(data, size, &record_offset,
                                                  ptr_buffer, sizeof(ptr_buffer));
            std::cout << "  PTR: " << std::string(ptr.str, ptr.length) << std::endl;
        }
    }
    
    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    
    std::cout << "Simple mDNS test - looking for _warpdeck._tcp services" << std::endl;
    
    // Open sockets
    int sock_ipv4 = mdns_socket_open_ipv4(nullptr);
    if (sock_ipv4 < 0) {
        std::cerr << "Failed to open IPv4 socket" << std::endl;
        return 1;
    }
    
    int sock_ipv6 = mdns_socket_open_ipv6(nullptr);
    if (sock_ipv6 < 0) {
        std::cerr << "Failed to open IPv6 socket" << std::endl;
        // Continue with just IPv4
    }
    
    std::cout << "Opened sockets: IPv4=" << sock_ipv4 << ", IPv6=" << sock_ipv6 << std::endl;
    
    // Send discovery query
    char buffer[1024];
    const char* service_name = "_warpdeck._tcp.local.";
    
    while (g_running) {
        // Send query
        std::cout << "\nSending discovery query..." << std::endl;
        
        if (sock_ipv4 >= 0) {
            int result = mdns_query_send(sock_ipv4, MDNS_RECORDTYPE_PTR, 
                                       service_name, strlen(service_name),
                                       buffer, sizeof(buffer), 0);
            if (result > 0) {
                std::cout << "Sent PTR query on IPv4 socket" << std::endl;
            }
        }
        
        if (sock_ipv6 >= 0) {
            int result = mdns_query_send(sock_ipv6, MDNS_RECORDTYPE_PTR,
                                       service_name, strlen(service_name),
                                       buffer, sizeof(buffer), 0);
            if (result > 0) {
                std::cout << "Sent PTR query on IPv6 socket" << std::endl;
            }
        }
        
        // Listen for responses for 3 seconds
        auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        
        while (std::chrono::steady_clock::now() < end_time && g_running) {
            fd_set readfs;
            FD_ZERO(&readfs);
            int max_fd = 0;
            
            if (sock_ipv4 >= 0) {
                FD_SET(sock_ipv4, &readfs);
                max_fd = sock_ipv4;
            }
            if (sock_ipv6 >= 0) {
                FD_SET(sock_ipv6, &readfs);
                if (sock_ipv6 > max_fd) max_fd = sock_ipv6;
            }
            
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms
            
            if (select(max_fd + 1, &readfs, nullptr, nullptr, &timeout) > 0) {
                if (sock_ipv4 >= 0 && FD_ISSET(sock_ipv4, &readfs)) {
                    size_t records = mdns_query_recv(sock_ipv4, buffer, sizeof(buffer),
                                                   query_callback, nullptr, 0);
                    if (records > 0) {
                        std::cout << "Received " << records << " records on IPv4" << std::endl;
                    }
                }
                
                if (sock_ipv6 >= 0 && FD_ISSET(sock_ipv6, &readfs)) {
                    size_t records = mdns_query_recv(sock_ipv6, buffer, sizeof(buffer),
                                                   query_callback, nullptr, 0);
                    if (records > 0) {
                        std::cout << "Received " << records << " records on IPv6" << std::endl;
                    }
                }
            }
        }
        
        if (g_running) {
            std::cout << "\nWaiting 2 seconds before next query..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    // Cleanup
    if (sock_ipv4 >= 0) mdns_socket_close(sock_ipv4);
    if (sock_ipv6 >= 0) mdns_socket_close(sock_ipv6);
    
    std::cout << "\nTest completed" << std::endl;
    return 0;
}