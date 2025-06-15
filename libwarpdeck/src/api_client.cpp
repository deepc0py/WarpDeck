#include "api_client.h"
#include "utils.h"
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <iomanip>
#include <sstream>

namespace warpdeck {

APIClient::APIClient() {}

APIClient::~APIClient() {}

APIResponse APIClient::get_device_info(const std::string& host, int port, 
                                     const std::string& /* expected_fingerprint */) {
    APIResponse response;
    
    try {
        // For now, use regular HTTP client - SSL will be implemented later
        httplib::Client client(host, port);
        
        auto result = client.Get("/api/v1/info");
        
        if (result) {
            response.status_code = result->status;
            response.body = result->body;
            response.success = (result->status == 200);
            
            if (!response.success) {
                response.error_message = "HTTP " + std::to_string(result->status);
            }
        } else {
            response.success = false;
            response.status_code = 0;
            response.error_message = "Connection failed";
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.status_code = 0;
        response.error_message = e.what();
    }
    
    return response;
}

APIResponse APIClient::request_transfer(const std::string& host, int port,
                                      const std::string& /* expected_fingerprint */,
                                      const TransferRequest& request) {
    APIResponse response;
    
    try {
        // For now, use regular HTTP client - SSL will be implemented later
        httplib::Client client(host, port);
        
        std::string json_body = utils::transfer_request_to_json(request);
        auto result = client.Post("/api/v1/transfer/request", json_body, "application/json");
        
        if (result) {
            response.status_code = result->status;
            response.body = result->body;
            response.success = (result->status == 202);
            
            if (!response.success) {
                response.error_message = "HTTP " + std::to_string(result->status);
            }
        } else {
            response.success = false;
            response.status_code = 0;
            response.error_message = "Connection failed";
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.status_code = 0;
        response.error_message = e.what();
    }
    
    return response;
}

APIResponse APIClient::upload_file(const std::string& host, int port,
                                 const std::string& expected_fingerprint,
                                 const std::string& transfer_id, int file_index,
                                 const std::vector<uint8_t>& file_data) {
    APIResponse response;
    
    try {
        // For now, use regular HTTP client - SSL will be implemented later
        httplib::Client client(host, port);
        
        std::string endpoint = "/api/v1/transfer/" + transfer_id + "/" + std::to_string(file_index);
        std::string data(file_data.begin(), file_data.end());
        
        auto result = client.Post(endpoint.c_str(), data, "application/octet-stream");
        
        if (result) {
            response.status_code = result->status;
            response.body = result->body;
            response.success = (result->status == 200);
            
            if (!response.success) {
                response.error_message = "HTTP " + std::to_string(result->status);
            }
        } else {
            response.success = false;
            response.status_code = 0;
            response.error_message = "Connection failed";
        }
        
    } catch (const std::exception& e) {
        response.success = false;
        response.status_code = 0;
        response.error_message = e.what();
    }
    
    return response;
}

void APIClient::set_client_certificate(const std::string& cert_file, const std::string& key_file) {
    client_cert_file_ = cert_file;
    client_key_file_ = key_file;
}

bool APIClient::verify_server_certificate(const std::string& expected_fingerprint, 
                                         const std::string& server_cert) {
    std::string actual_fingerprint = calculate_certificate_fingerprint(server_cert);
    return actual_fingerprint == expected_fingerprint;
}

std::string APIClient::calculate_certificate_fingerprint(const std::string& cert_pem) {
    BIO* bio = BIO_new_mem_buf(cert_pem.c_str(), -1);
    if (!bio) {
        return "";
    }
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) {
        return "";
    }
    
    unsigned char* cert_der = nullptr;
    int cert_der_len = i2d_X509(cert, &cert_der);
    X509_free(cert);
    
    if (cert_der_len <= 0) {
        return "";
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(cert_der, cert_der_len, hash);
    OPENSSL_free(cert_der);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

} // namespace warpdeck