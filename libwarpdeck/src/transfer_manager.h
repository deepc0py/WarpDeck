#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include "api_server.h"

namespace warpdeck {

enum class TransferDirection {
    SENDING,
    RECEIVING
};

enum class TransferStatus {
    PENDING_APPROVAL,
    APPROVED,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct TransferInfo {
    std::string transfer_id;
    std::string peer_device_id;
    std::string peer_name;
    TransferDirection direction;
    TransferStatus status;
    std::vector<FileMetadata> files;
    uint64_t total_bytes;
    uint64_t transferred_bytes;
    std::string error_message;
    std::string destination_folder;
};

class TransferManager {
public:
    using ProgressCallback = std::function<void(const std::string& transfer_id, float progress_percent, uint64_t bytes_transferred)>;
    using CompletionCallback = std::function<void(const std::string& transfer_id, bool success, const std::string& error_message)>;
    using IncomingRequestCallback = std::function<void(const std::string& transfer_id, const std::string& peer_name, const std::vector<FileMetadata>& files)>;

    TransferManager();
    ~TransferManager();

    void set_download_folder(const std::string& folder);
    void set_progress_callback(ProgressCallback callback);
    void set_completion_callback(CompletionCallback callback);
    void set_incoming_request_callback(IncomingRequestCallback callback);

    // Outgoing transfers
    std::string initiate_transfer(const std::string& peer_device_id, const std::string& peer_name,
                                 const std::vector<std::string>& file_paths);
    
    // Incoming transfers
    std::string handle_incoming_request(const std::string& peer_device_id, const std::string& peer_name,
                                       const TransferRequest& request);
    void respond_to_transfer(const std::string& transfer_id, bool accept);
    
    // File upload handling
    bool handle_file_upload(const std::string& transfer_id, int file_index, const std::vector<uint8_t>& data);
    
    // Transfer management
    void cancel_transfer(const std::string& transfer_id);
    std::map<std::string, TransferInfo> get_active_transfers() const;
    TransferInfo get_transfer_info(const std::string& transfer_id) const;

private:
    std::string generate_transfer_id();
    bool create_temporary_file(const std::string& transfer_id, int file_index);
    bool finalize_received_file(const std::string& transfer_id, int file_index);
    void cleanup_transfer(const std::string& transfer_id);
    void update_transfer_progress(const std::string& transfer_id);
    
    mutable std::mutex transfers_mutex_;
    std::map<std::string, TransferInfo> active_transfers_;
    std::map<std::string, std::vector<std::string>> temp_file_paths_;
    
    std::string download_folder_;
    ProgressCallback progress_callback_;
    CompletionCallback completion_callback_;
    IncomingRequestCallback incoming_request_callback_;
};

} // namespace warpdeck