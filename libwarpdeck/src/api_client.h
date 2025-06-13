#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "api_server.h"

namespace warpdeck {

struct APIResponse {
    int status_code;
    std::string body;
    bool success;
    std::string error_message;
};

class APIClient {
public:
    APIClient();
    ~APIClient();

    // Device info endpoint
    APIResponse get_device_info(const std::string& host, int port, 
                               const std::string& expected_fingerprint);
    
    // Transfer request endpoint
    APIResponse request_transfer(const std::string& host, int port, 
                                const std::string& expected_fingerprint,
                                const TransferRequest& request);
    
    // File upload endpoint
    APIResponse upload_file(const std::string& host, int port,
                           const std::string& expected_fingerprint,
                           const std::string& transfer_id, int file_index,
                           const std::vector<uint8_t>& file_data);

    void set_client_certificate(const std::string& cert_file, const std::string& key_file);

private:
    bool verify_server_certificate(const std::string& expected_fingerprint, 
                                  const std::string& server_cert);
    std::string calculate_certificate_fingerprint(const std::string& cert_pem);
    
    std::string client_cert_file_;
    std::string client_key_file_;
};

} // namespace warpdeck