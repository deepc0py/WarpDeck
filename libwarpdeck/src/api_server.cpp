#include "api_server.h"
#include "utils.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <iostream>
#include <ctime>

namespace warpdeck {

APIServer::APIServer() : port_(0), running_(false) {}

APIServer::~APIServer() {
    stop();
}

bool APIServer::start(int port, const DeviceInfo& device_info) {
    if (running_) {
        return false;
    }
    
    device_info_ = device_info;
    
    // For now, use regular HTTP server - SSL will be implemented later
    server_ = std::make_unique<httplib::Server>();
    if (!server_) {
        return false;
    }
    
    // Setup routes
    setup_routes();
    
    // Find available port if none specified
    if (port == 0) {
        for (int p = 54321; p < 65535; ++p) {
            server_->set_read_timeout(1, 0);  // 1 second timeout for quick startup
            server_->set_write_timeout(1, 0);
            
            // Test if port is available by trying to bind
            if (server_->bind_to_port("0.0.0.0", p)) {
                port_ = p;
                std::cout << "Successfully bound to port " << port_ << std::endl;
                break;
            }
        }
        if (port_ == 0) {
            std::cerr << "No available port found in range 54321-65534" << std::endl;
            return false; // No available port found
        }
    } else {
        port_ = port;
        if (!server_->bind_to_port("0.0.0.0", port_)) {
            std::cerr << "Failed to bind to specified port " << port_ << std::endl;
            return false; // Failed to bind to specified port
        }
        std::cout << "Successfully bound to specified port " << port_ << std::endl;
    }
    
    // Start server in background thread
    std::thread server_thread([this]() {
        try {
            server_->listen_after_bind();
        } catch (const std::exception& e) {
            std::cerr << "Server error: " << e.what() << std::endl;
            running_ = false;
        }
    });
    server_thread.detach();
    
    // Mark as running since bind was successful
    running_ = true;
    return true;
}

void APIServer::stop() {
    if (running_ && server_) {
        server_->stop();
        running_ = false;
    }
}

int APIServer::get_port() const {
    return port_;
}

void APIServer::set_transfer_request_callback(TransferRequestCallback callback) {
    transfer_request_callback_ = callback;
}

void APIServer::set_file_upload_callback(FileUploadCallback callback) {
    file_upload_callback_ = callback;
}

void APIServer::set_ssl_certificate(const std::string& cert_file, const std::string& key_file) {
    // Store certificate paths for use when creating the server
    cert_file_ = cert_file;
    key_file_ = key_file;
}

void APIServer::setup_routes() {
    if (!server_) {
        return;
    }
    
    // GET /health - Health check endpoint
    server_->Get("/health", [this](const httplib::Request& /* req */, httplib::Response& res) {
        try {
            nlohmann::json health_response;
            health_response["status"] = "healthy";
            health_response["service"] = "WarpDeck Core Service";
            health_response["timestamp"] = std::time(nullptr);
            health_response["port"] = port_;
            
            res.set_content(health_response.dump(), "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"status\":\"unhealthy\",\"error\":\"Internal server error\"}", "application/json");
        }
    });
    
    // GET /api/v1/info - Device information endpoint
    server_->Get("/api/v1/info", [this](const httplib::Request& /* req */, httplib::Response& res) {
        try {
            std::string json = utils::device_info_to_json(device_info_);
            res.set_content(json, "application/json");
            res.status = 200;
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal server error\"}", "application/json");
        }
    });
    
    // POST /api/v1/transfer/request - Transfer request endpoint
    server_->Post("/api/v1/transfer/request", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            // Parse request body
            TransferRequest transfer_req;
            if (!utils::parse_transfer_request(req.body, transfer_req)) {
                res.status = 400;
                res.set_content("{\"error_code\":\"INVALID_REQUEST\",\"message\":\"Invalid request format\"}", 
                               "application/json");
                return;
            }
            
            // Extract client certificate fingerprint
            std::string client_fingerprint = extract_client_fingerprint_from_ssl();
            
            // Handle through callback
            if (transfer_request_callback_) {
                transfer_request_callback_(client_fingerprint, transfer_req, 
                    [&res](bool approved, const std::string& transfer_id) {
                        if (approved) {
                            TransferSession session;
                            session.transfer_id = transfer_id;
                            session.status = "ready_to_receive";
                            session.expires_at = utils::get_expiry_timestamp(30);
                            
                            nlohmann::json response_json;
                            response_json["transfer_id"] = session.transfer_id;
                            response_json["status"] = session.status;
                            response_json["expires_at"] = session.expires_at;
                            
                            res.status = 202;
                            res.set_content(response_json.dump(), "application/json");
                        } else {
                            res.status = 403;
                            res.set_content("{\"error_code\":\"USER_DECLINED\",\"message\":\"Transfer declined by user\"}", 
                                           "application/json");
                        }
                    });
            } else {
                res.status = 500;
                res.set_content("{\"error_code\":\"SERVER_ERROR\",\"message\":\"No transfer handler configured\"}", 
                               "application/json");
            }
            
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error_code\":\"SERVER_ERROR\",\"message\":\"Internal server error\"}", 
                           "application/json");
        }
    });
    
    // POST /api/v1/transfer/{transfer_id}/{file_index} - File upload endpoint
    server_->Post(R"(/api/v1/transfer/([^/]+)/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string transfer_id = req.matches[1];
            int file_index = std::stoi(req.matches[2]);
            
            // Handle through callback
            if (file_upload_callback_) {
                file_upload_callback_(transfer_id, file_index, req.body,
                    [&res](bool success, const std::string& error) {
                        if (success) {
                            res.status = 200;
                        } else {
                            res.status = 500;
                            nlohmann::json error_json;
                            error_json["error_code"] = "UPLOAD_FAILED";
                            error_json["message"] = error.empty() ? "Upload failed" : error;
                            res.set_content(error_json.dump(), "application/json");
                        }
                    });
            } else {
                res.status = 500;
                res.set_content("{\"error_code\":\"SERVER_ERROR\",\"message\":\"No upload handler configured\"}", 
                               "application/json");
            }
            
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error_code\":\"SERVER_ERROR\",\"message\":\"Internal server error\"}", 
                           "application/json");
        }
    });
    
    // Set error handler
    server_->set_error_handler([](const httplib::Request& /* req */, httplib::Response& res) {
        res.status = 404;
        res.set_content("{\"error_code\":\"NOT_FOUND\",\"message\":\"Endpoint not found\"}", 
                       "application/json");
    });
}

std::string APIServer::extract_client_fingerprint_from_ssl() {
    // This is a simplified implementation
    // In a real implementation, this would extract the client certificate
    // from the TLS session and calculate its SHA-256 fingerprint
    return "placeholder_fingerprint";
}

} // namespace warpdeck