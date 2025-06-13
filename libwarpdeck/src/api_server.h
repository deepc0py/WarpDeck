#pragma once

#include <string>
#include <functional>
#include <memory>
#include <httplib.h>

namespace warpdeck {

struct DeviceInfo {
    std::string id;
    std::string name;
    std::string platform;
    std::string protocol_version;
};

struct FileMetadata {
    std::string name;
    uint64_t size;
    std::string hash; // optional SHA256 hash
};

struct TransferRequest {
    std::vector<FileMetadata> files;
};

struct TransferSession {
    std::string transfer_id;
    std::string status;
    std::string expires_at;
};

class APIServer {
public:
    using TransferRequestCallback = std::function<void(const std::string& client_fingerprint, 
                                                       const TransferRequest& request,
                                                       std::function<void(bool approved, const std::string& transfer_id)> response_callback)>;
    using FileUploadCallback = std::function<void(const std::string& transfer_id, 
                                                   int file_index, 
                                                   const std::string& data,
                                                   std::function<void(bool success, const std::string& error)> response_callback)>;

    APIServer();
    ~APIServer();

    bool start(int port, const DeviceInfo& device_info);
    void stop();
    int get_port() const;
    
    void set_transfer_request_callback(TransferRequestCallback callback);
    void set_file_upload_callback(FileUploadCallback callback);
    
    void set_ssl_certificate(const std::string& cert_file, const std::string& key_file);

private:
    void setup_routes();
    std::string extract_client_fingerprint_from_ssl();
    
    std::unique_ptr<httplib::Server> server_;
    DeviceInfo device_info_;
    int port_;
    bool running_;
    
    std::string cert_file_;
    std::string key_file_;
    
    TransferRequestCallback transfer_request_callback_;
    FileUploadCallback file_upload_callback_;
};

} // namespace warpdeck