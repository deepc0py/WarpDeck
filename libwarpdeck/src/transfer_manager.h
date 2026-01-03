#pragma once

#include <string>
#include <map>
#include <vector>
#include <deque>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>
#include "api_server.h"

namespace warpdeck {

class APIClient;

// Peer info for sending transfers
struct PeerConnectionInfo {
    std::string host;
    int port;
    std::string fingerprint;
};

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

// Queue status for transfers waiting to be processed
enum class QueuedTransferStatus {
    QUEUED,      // Waiting in queue
    ACTIVE,      // Currently transferring
    COMPLETED,   // Successfully completed
    FAILED,      // Failed with error
    CANCELLED    // Cancelled by user
};

// A transfer waiting in the queue
struct QueuedTransfer {
    std::string queue_id;                          // Unique queue entry ID
    std::string peer_device_id;
    std::string peer_name;
    std::vector<std::string> file_paths;
    std::vector<std::string> relative_paths;       // For folder transfers
    QueuedTransferStatus status;
    std::string transfer_id;                       // Filled when transfer becomes active
    std::string error_message;
    std::chrono::system_clock::time_point queued_at;
    uint64_t total_bytes;                          // Pre-calculated total size
    int file_count;
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
    using QueueStatusCallback = std::function<void(const std::string& queue_id, int position, int total)>;
    using PeerLookupCallback = std::function<PeerConnectionInfo(const std::string& device_id)>;

    TransferManager();
    ~TransferManager();

    // Set dependencies for sending
    void set_api_client(APIClient* client);
    void set_peer_lookup(PeerLookupCallback callback);

    void set_download_folder(const std::string& folder);
    void set_progress_callback(ProgressCallback callback);
    void set_completion_callback(CompletionCallback callback);
    void set_incoming_request_callback(IncomingRequestCallback callback);
    void set_queue_status_callback(QueueStatusCallback callback);

    // Outgoing transfers (direct - starts immediately)
    std::string initiate_transfer(const std::string& peer_device_id, const std::string& peer_name,
                                 const std::vector<std::string>& file_paths,
                                 const std::vector<std::string>& relative_paths = {});

    // Outgoing transfers (queued - waits for previous transfers to complete)
    std::string queue_transfer(const std::string& peer_device_id, const std::string& peer_name,
                              const std::vector<std::string>& file_paths,
                              const std::vector<std::string>& relative_paths = {});

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

    // Queue management
    bool cancel_queued_transfer(const std::string& queue_id);
    std::vector<QueuedTransfer> get_queue_status() const;

private:
    std::string generate_transfer_id();
    bool create_temporary_file(const std::string& transfer_id, int file_index);
    bool finalize_received_file(const std::string& transfer_id, int file_index);
    void cleanup_transfer(const std::string& transfer_id);
    void update_transfer_progress(const std::string& transfer_id);

    // Queue processing
    void process_next_in_queue();
    void on_transfer_finished(const std::string& transfer_id, bool success, const std::string& error_message);
    void notify_queue_positions();
    void cleanup_old_queue_entries();

    // Actual file sending (runs in background thread)
    void execute_send_transfer(const std::string& transfer_id);

    mutable std::mutex transfers_mutex_;
    std::map<std::string, TransferInfo> active_transfers_;
    std::map<std::string, std::vector<std::string>> temp_file_paths_;
    std::map<std::string, std::vector<std::string>> send_file_paths_;  // Original paths for sending
    std::map<std::string, std::thread> send_threads_;  // Background send threads

    // Queue members
    mutable std::mutex queue_mutex_;
    std::deque<QueuedTransfer> transfer_queue_;
    std::string current_queue_id_;  // Queue ID of currently active transfer

    std::string download_folder_;
    ProgressCallback progress_callback_;
    CompletionCallback completion_callback_;
    IncomingRequestCallback incoming_request_callback_;
    QueueStatusCallback queue_status_callback_;
    PeerLookupCallback peer_lookup_callback_;
    APIClient* api_client_ = nullptr;
};

} // namespace warpdeck