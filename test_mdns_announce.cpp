#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <vector>

// Direct test of mDNS announcement
extern "C" {
#include "third_party/mjansson_mdns/mdns.h"
}

static volatile bool g_running = true;

void signal_handler(int sig) {
    g_running = false;
}

int main() {
    signal(SIGINT, signal_handler);
    
    std::cout << "mDNS Announcement Test - Publishing _warpdeck._tcp service" << std::endl;
    
    // Open sockets for publishing (bind to mDNS port)
    struct sockaddr_in service_addr_ipv4;
    memset(&service_addr_ipv4, 0, sizeof(service_addr_ipv4));
    service_addr_ipv4.sin_family = AF_INET;
    service_addr_ipv4.sin_addr.s_addr = INADDR_ANY;
    service_addr_ipv4.sin_port = htons(MDNS_PORT);
    
    int sock_ipv4 = mdns_socket_open_ipv4(&service_addr_ipv4);
    if (sock_ipv4 < 0) {
        std::cerr << "Failed to open IPv4 socket" << std::endl;
        return 1;
    }
    
    std::cout << "Opened IPv4 socket: " << sock_ipv4 << std::endl;
    
    // Service information
    std::string device_id = "test-device-" + std::to_string(getpid());
    std::string service_instance = device_id + "._warpdeck._tcp.local.";
    std::string service_type = "_warpdeck._tcp.local.";
    std::string hostname = "testhost.local.";
    int port = 54321;
    
    char buffer[2048];
    
    while (g_running) {
        std::cout << "\nSending service announcement..." << std::endl;
        
        // Create announcement records
        mdns_record_t records[3];
        
        // PTR record
        records[0].name.str = service_type.c_str();
        records[0].name.length = service_type.length();
        records[0].type = MDNS_RECORDTYPE_PTR;
        records[0].rclass = 0;
        records[0].ttl = 120;
        records[0].data.ptr.name.str = service_instance.c_str();
        records[0].data.ptr.name.length = service_instance.length();
        
        // SRV record
        records[1].name.str = service_instance.c_str();
        records[1].name.length = service_instance.length();
        records[1].type = MDNS_RECORDTYPE_SRV;
        records[1].rclass = 0;
        records[1].ttl = 120;
        records[1].data.srv.priority = 0;
        records[1].data.srv.weight = 0;
        records[1].data.srv.port = port;
        records[1].data.srv.name.str = hostname.c_str();
        records[1].data.srv.name.length = hostname.length();
        
        // TXT record
        records[2].name.str = service_instance.c_str();
        records[2].name.length = service_instance.length();
        records[2].type = MDNS_RECORDTYPE_TXT;
        records[2].rclass = 0;
        records[2].ttl = 120;
        records[2].data.txt.key.str = "deviceid";
        records[2].data.txt.key.length = 8;
        records[2].data.txt.value.str = device_id.c_str();
        records[2].data.txt.value.length = device_id.length();
        
        // Send announcement
        int result = mdns_announce_multicast(sock_ipv4, buffer, sizeof(buffer),
                                           records[0], nullptr, 0,
                                           &records[1], 2);
        
        if (result < 0) {
            std::cerr << "Failed to send announcement: " << result << std::endl;
        } else {
            std::cout << "Announcement sent successfully" << std::endl;
            std::cout << "  Service: " << service_instance << std::endl;
            std::cout << "  Port: " << port << std::endl;
        }
        
        // Also listen for queries
        std::cout << "Listening for queries for 5 seconds..." << std::endl;
        auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        
        while (std::chrono::steady_clock::now() < end_time && g_running) {
            fd_set readfs;
            FD_ZERO(&readfs);
            FD_SET(sock_ipv4, &readfs);
            
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms
            
            if (select(sock_ipv4 + 1, &readfs, nullptr, nullptr, &timeout) > 0) {
                if (FD_ISSET(sock_ipv4, &readfs)) {
                    size_t records_rcv = mdns_socket_listen(sock_ipv4, buffer, sizeof(buffer),
                        [](int sock, const struct sockaddr* from, size_t addrlen,
                           mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
                           uint16_t rclass, uint32_t ttl, const void* data, size_t size,
                           size_t name_offset, size_t name_length, size_t record_offset,
                           size_t record_length, void* user_data) -> int {
                            
                            if (entry == MDNS_ENTRYTYPE_QUESTION) {
                                char name_buffer[256];
                                mdns_string_t name = mdns_string_extract(data, size, &name_offset,
                                                                       name_buffer, sizeof(name_buffer));
                                std::string query_name(name.str, name.length);
                                
                                if (query_name.find("_warpdeck._tcp") != std::string::npos) {
                                    std::cout << "Received query for _warpdeck service, type: " << rtype << std::endl;
                                }
                            }
                            return 0;
                        }, nullptr);
                    
                    if (records_rcv > 0) {
                        std::cout << "Processed " << records_rcv << " incoming records" << std::endl;
                    }
                }
            }
        }
        
        if (g_running) {
            std::cout << "\nWaiting before next announcement..." << std::endl;
        }
    }
    
    // Send goodbye
    std::cout << "\nSending goodbye..." << std::endl;
    mdns_record_t goodbye_records[3];
    
    // PTR record goodbye
    goodbye_records[0].name.str = service_type.c_str();
    goodbye_records[0].name.length = service_type.length();
    goodbye_records[0].type = MDNS_RECORDTYPE_PTR;
    goodbye_records[0].data.ptr.name.str = service_instance.c_str();
    goodbye_records[0].data.ptr.name.length = service_instance.length();
    
    for (int i = 0; i < 1; i++) {
        mdns_goodbye_multicast(sock_ipv4, buffer, sizeof(buffer),
                             goodbye_records[i], nullptr, 0, nullptr, 0);
    }
    
    // Cleanup
    mdns_socket_close(sock_ipv4);
    
    std::cout << "Test completed" << std::endl;
    return 0;
}