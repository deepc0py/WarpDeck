#include "transfer_manager.h"
#include "api_client.h"
#include "utils.h"
#include "logger.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace warpdeck {

TransferManager::TransferManager() {
    download_folder_ = utils::get_default_download_dir();
}

TransferManager::~TransferManager() {
    // Wait for any active send threads to complete
    for (auto& [id, thread] : send_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void TransferManager::set_api_client(APIClient* client) {
    api_client_ = client;
}

void TransferManager::set_peer_lookup(PeerLookupCallback callback) {
    peer_lookup_callback_ = callback;
}

void TransferManager::set_download_folder(const std::string& folder) {
    download_folder_ = folder;
}

void TransferManager::set_progress_callback(ProgressCallback callback) {
    progress_callback_ = callback;
}

void TransferManager::set_completion_callback(CompletionCallback callback) {
    // Wrap the callback to also trigger queue processing when transfers complete
    completion_callback_ = [this, callback](const std::string& transfer_id, bool success, const std::string& error) {
        // Call the original callback first
        if (callback) {
            callback(transfer_id, success, error);
        }
        // Then process the queue
        on_transfer_finished(transfer_id, success, error);
    };
}

void TransferManager::set_incoming_request_callback(IncomingRequestCallback callback) {
    incoming_request_callback_ = callback;
}

void TransferManager::set_queue_status_callback(QueueStatusCallback callback) {
    queue_status_callback_ = callback;
}

std::string TransferManager::initiate_transfer(const std::string& peer_device_id, const std::string& peer_name,
                                              const std::vector<std::string>& file_paths,
                                              const std::vector<std::string>& relative_paths) {
    std::string transfer_id = generate_transfer_id();

    TransferInfo transfer;
    transfer.transfer_id = transfer_id;
    transfer.peer_device_id = peer_device_id;
    transfer.peer_name = peer_name;
    transfer.direction = TransferDirection::SENDING;
    transfer.status = TransferStatus::IN_PROGRESS;
    transfer.total_bytes = 0;
    transfer.transferred_bytes = 0;

    std::vector<std::string> valid_file_paths;

    // Build file metadata
    for (size_t i = 0; i < file_paths.size(); ++i) {
        const auto& file_path = file_paths[i];
        if (!utils::file_exists(file_path)) {
            LOG_CORE_WARN() << "File does not exist: " << file_path;
            continue;
        }

        FileMetadata file_meta;
        file_meta.name = utils::get_filename(file_path);
        file_meta.size = utils::get_file_size(file_path);
        file_meta.hash = utils::calculate_file_hash(file_path);

        // Set relative_path if provided (for folder transfers)
        if (i < relative_paths.size() && !relative_paths[i].empty()) {
            file_meta.relative_path = relative_paths[i];
        }

        transfer.files.push_back(file_meta);
        transfer.total_bytes += file_meta.size;
        valid_file_paths.push_back(file_path);
    }

    if (transfer.files.empty()) {
        LOG_CORE_ERROR() << "No valid files to transfer";
        return ""; // No valid files
    }

    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        active_transfers_[transfer_id] = transfer;
        send_file_paths_[transfer_id] = valid_file_paths;
    }

    LOG_CORE_INFO() << "Initiating transfer " << transfer_id << " to " << peer_name
                    << " with " << transfer.files.size() << " files";

    // Start the actual send in a background thread
    send_threads_[transfer_id] = std::thread(&TransferManager::execute_send_transfer, this, transfer_id);

    return transfer_id;
}

std::string TransferManager::handle_incoming_request(const std::string& peer_device_id, const std::string& peer_name,
                                                    const TransferRequest& request) {
    std::string transfer_id = generate_transfer_id();
    
    TransferInfo transfer;
    transfer.transfer_id = transfer_id;
    transfer.peer_device_id = peer_device_id;
    transfer.peer_name = peer_name;
    transfer.direction = TransferDirection::RECEIVING;
    transfer.status = TransferStatus::PENDING_APPROVAL;
    transfer.files = request.files;
    transfer.total_bytes = 0;
    transfer.transferred_bytes = 0;
    transfer.destination_folder = download_folder_;
    
    // Calculate total bytes
    for (const auto& file : transfer.files) {
        transfer.total_bytes += file.size;
    }
    
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        active_transfers_[transfer_id] = transfer;
    }
    
    // Notify UI about incoming request
    if (incoming_request_callback_) {
        incoming_request_callback_(transfer_id, peer_name, transfer.files);
    }
    
    return transfer_id;
}

void TransferManager::respond_to_transfer(const std::string& transfer_id, bool accept) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = active_transfers_.find(transfer_id);
    if (it == active_transfers_.end()) {
        return;
    }
    
    TransferInfo& transfer = it->second;
    
    if (accept) {
        transfer.status = TransferStatus::APPROVED;
        
        // Create temporary files for receiving
        if (transfer.direction == TransferDirection::RECEIVING) {
            for (size_t i = 0; i < transfer.files.size(); ++i) {
                create_temporary_file(transfer_id, static_cast<int>(i));
            }
        }
    } else {
        transfer.status = TransferStatus::CANCELLED;
        cleanup_transfer(transfer_id);
        
        if (completion_callback_) {
            completion_callback_(transfer_id, false, "Transfer declined");
        }
    }
}

bool TransferManager::handle_file_upload(const std::string& transfer_id, int file_index, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = active_transfers_.find(transfer_id);
    if (it == active_transfers_.end()) {
        return false;
    }
    
    TransferInfo& transfer = it->second;
    
    if (transfer.direction != TransferDirection::RECEIVING || 
        transfer.status != TransferStatus::APPROVED ||
        file_index >= static_cast<int>(transfer.files.size())) {
        return false;
    }
    
    // Write data to temporary file
    auto temp_it = temp_file_paths_.find(transfer_id);
    if (temp_it == temp_file_paths_.end() || 
        file_index >= static_cast<int>(temp_it->second.size())) {
        return false;
    }
    
    const std::string& temp_path = temp_it->second[file_index];
    
    try {
        std::ofstream file(temp_path, std::ios::binary | std::ios::app);
        if (!file) {
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        
        // Update progress
        transfer.transferred_bytes += data.size();
        update_transfer_progress(transfer_id);
        
        // Check if file is complete
        uint64_t current_size = utils::get_file_size(temp_path);
        if (current_size >= transfer.files[file_index].size) {
            // File complete, move to final destination
            if (finalize_received_file(transfer_id, file_index)) {
                // Check if all files are complete
                bool all_complete = true;
                for (size_t i = 0; i < transfer.files.size(); ++i) {
                    const FileMetadata& file_meta = transfer.files[i];
                    std::string final_path;
                    if (!file_meta.relative_path.empty()) {
                        std::string safe_relative = utils::sanitize_relative_path(file_meta.relative_path);
                        final_path = transfer.destination_folder + "/" + safe_relative;
                    } else {
                        std::string safe_name = utils::sanitize_filename(file_meta.name);
                        final_path = transfer.destination_folder + "/" + safe_name;
                    }
                    if (!utils::file_exists(final_path)) {
                        all_complete = false;
                        break;
                    }
                }

                if (all_complete) {
                    transfer.status = TransferStatus::COMPLETED;
                    cleanup_transfer(transfer_id);

                    if (completion_callback_) {
                        completion_callback_(transfer_id, true, "");
                    }
                }
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error writing file: " << e.what() << std::endl;
        return false;
    }
}

void TransferManager::cancel_transfer(const std::string& transfer_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = active_transfers_.find(transfer_id);
    if (it != active_transfers_.end()) {
        it->second.status = TransferStatus::CANCELLED;
        cleanup_transfer(transfer_id);
        
        if (completion_callback_) {
            completion_callback_(transfer_id, false, "Transfer cancelled");
        }
    }
}

std::map<std::string, TransferInfo> TransferManager::get_active_transfers() const {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    return active_transfers_;
}

TransferInfo TransferManager::get_transfer_info(const std::string& transfer_id) const {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = active_transfers_.find(transfer_id);
    if (it != active_transfers_.end()) {
        return it->second;
    }
    return TransferInfo{}; // Return empty info if not found
}

std::string TransferManager::generate_transfer_id() {
    return utils::generate_uuid();
}

bool TransferManager::create_temporary_file(const std::string& transfer_id, int file_index) {
    std::string temp_dir = download_folder_ + "/.warpdeck_temp";
    if (!utils::create_directory(temp_dir)) {
        return false;
    }
    
    std::string temp_filename = transfer_id + "_" + std::to_string(file_index) + ".tmp";
    std::string temp_path = temp_dir + "/" + temp_filename;
    
    // Create empty temporary file
    std::ofstream file(temp_path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.close();
    
    // Store temporary file path
    temp_file_paths_[transfer_id].resize(std::max(static_cast<size_t>(file_index + 1), 
                                                  temp_file_paths_[transfer_id].size()));
    temp_file_paths_[transfer_id][file_index] = temp_path;
    
    return true;
}

bool TransferManager::finalize_received_file(const std::string& transfer_id, int file_index) {
    auto temp_it = temp_file_paths_.find(transfer_id);
    if (temp_it == temp_file_paths_.end() ||
        file_index >= static_cast<int>(temp_it->second.size())) {
        return false;
    }

    const std::string& temp_path = temp_it->second[file_index];

    auto transfer_it = active_transfers_.find(transfer_id);
    if (transfer_it == active_transfers_.end() ||
        file_index >= static_cast<int>(transfer_it->second.files.size())) {
        return false;
    }

    const FileMetadata& file_meta = transfer_it->second.files[file_index];
    std::string final_path;

    if (!file_meta.relative_path.empty()) {
        // Folder transfer: use relative_path to preserve directory structure
        std::string safe_relative = utils::sanitize_relative_path(file_meta.relative_path);
        final_path = transfer_it->second.destination_folder + "/" + safe_relative;
    } else {
        // Flat file transfer: use sanitized filename only (backwards compatible)
        std::string safe_name = utils::sanitize_filename(file_meta.name);
        final_path = transfer_it->second.destination_folder + "/" + safe_name;
    }

    try {
        // Ensure destination directory exists (creates parent dirs for folder transfers)
        std::string dest_dir = utils::get_parent_directory(final_path);
        if (!utils::create_directory(dest_dir)) {
            return false;
        }

        // Move temporary file to final destination
        std::filesystem::rename(temp_path, final_path);

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error finalizing file: " << e.what() << std::endl;
        return false;
    }
}

void TransferManager::cleanup_transfer(const std::string& transfer_id) {
    // Remove temporary files
    auto temp_it = temp_file_paths_.find(transfer_id);
    if (temp_it != temp_file_paths_.end()) {
        for (const auto& temp_path : temp_it->second) {
            try {
                if (utils::file_exists(temp_path)) {
                    std::filesystem::remove(temp_path);
                }
            } catch (const std::exception&) {
                // Ignore cleanup errors
            }
        }
        temp_file_paths_.erase(temp_it);
    }
    
    // Remove from active transfers
    active_transfers_.erase(transfer_id);
}

void TransferManager::update_transfer_progress(const std::string& transfer_id) {
    auto it = active_transfers_.find(transfer_id);
    if (it != active_transfers_.end() && progress_callback_) {
        const TransferInfo& transfer = it->second;
        float progress = transfer.total_bytes > 0 ?
            (static_cast<float>(transfer.transferred_bytes) / transfer.total_bytes) * 100.0f : 0.0f;

        progress_callback_(transfer_id, progress, transfer.transferred_bytes);
    }
}

// ============================================================================
// Queue Implementation
// ============================================================================

std::string TransferManager::queue_transfer(const std::string& peer_device_id, const std::string& peer_name,
                                           const std::vector<std::string>& file_paths,
                                           const std::vector<std::string>& relative_paths) {
    QueuedTransfer queued;
    queued.queue_id = generate_transfer_id();
    queued.peer_device_id = peer_device_id;
    queued.peer_name = peer_name;
    queued.file_paths = file_paths;
    queued.relative_paths = relative_paths;
    queued.status = QueuedTransferStatus::QUEUED;
    queued.queued_at = std::chrono::system_clock::now();
    queued.file_count = static_cast<int>(file_paths.size());

    // Calculate total bytes
    queued.total_bytes = 0;
    for (const auto& path : file_paths) {
        if (utils::file_exists(path)) {
            queued.total_bytes += utils::get_file_size(path);
        }
    }

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        transfer_queue_.push_back(queued);
    }

    // Notify about queue update
    notify_queue_positions();

    // Try to start processing if nothing is active
    process_next_in_queue();

    return queued.queue_id;
}

bool TransferManager::cancel_queued_transfer(const std::string& queue_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    for (auto it = transfer_queue_.begin(); it != transfer_queue_.end(); ++it) {
        if (it->queue_id == queue_id) {
            if (it->status == QueuedTransferStatus::QUEUED) {
                // Not started yet, just remove from queue
                transfer_queue_.erase(it);
                notify_queue_positions();
                return true;
            } else if (it->status == QueuedTransferStatus::ACTIVE) {
                // Currently active, cancel the underlying transfer
                if (!it->transfer_id.empty()) {
                    // Need to release queue lock before calling cancel_transfer (which locks transfers_mutex_)
                    std::string transfer_id = it->transfer_id;
                    it->status = QueuedTransferStatus::CANCELLED;
                    current_queue_id_.clear();

                    // Unlock queue mutex, cancel transfer, then process next
                    lock.~lock_guard();  // Manual unlock
                    cancel_transfer(transfer_id);

                    // Reacquire and process next
                    std::lock_guard<std::mutex> new_lock(queue_mutex_);
                    // process_next_in_queue will be called by on_transfer_finished
                }
                return true;
            }
            break;
        }
    }
    return false;
}

std::vector<QueuedTransfer> TransferManager::get_queue_status() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return std::vector<QueuedTransfer>(transfer_queue_.begin(), transfer_queue_.end());
}

void TransferManager::process_next_in_queue() {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    // Check if there's already an active transfer
    if (!current_queue_id_.empty()) {
        return;  // Wait for current transfer to finish
    }

    // Find next queued transfer
    for (auto& queued : transfer_queue_) {
        if (queued.status == QueuedTransferStatus::QUEUED) {
            // Start this transfer
            queued.status = QueuedTransferStatus::ACTIVE;
            current_queue_id_ = queued.queue_id;

            // Actually initiate the transfer (releases queue lock temporarily)
            // We need to copy data since we're about to unlock
            std::string peer_device_id = queued.peer_device_id;
            std::string peer_name = queued.peer_name;
            std::vector<std::string> file_paths = queued.file_paths;
            std::vector<std::string> relative_paths = queued.relative_paths;

            // Unlock before calling initiate_transfer
            lock.~lock_guard();

            std::string transfer_id = initiate_transfer(peer_device_id, peer_name, file_paths, relative_paths);

            // Reacquire lock and update transfer_id
            std::lock_guard<std::mutex> new_lock(queue_mutex_);
            for (auto& q : transfer_queue_) {
                if (q.queue_id == current_queue_id_) {
                    q.transfer_id = transfer_id;
                    break;
                }
            }

            // Notify queue position updates
            notify_queue_positions();
            return;
        }
    }
}

void TransferManager::on_transfer_finished(const std::string& transfer_id, bool success, const std::string& error_message) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        // Find and update the queued entry
        for (auto& queued : transfer_queue_) {
            if (queued.transfer_id == transfer_id) {
                queued.status = success ? QueuedTransferStatus::COMPLETED : QueuedTransferStatus::FAILED;
                queued.error_message = error_message;
                break;
            }
        }

        // Clear current active marker
        current_queue_id_.clear();

        // Clean up old completed/failed entries
        cleanup_old_queue_entries();
    }

    // Process next in queue (outside lock)
    process_next_in_queue();
}

void TransferManager::notify_queue_positions() {
    if (!queue_status_callback_) {
        return;
    }

    // Count only queued items (not completed/failed)
    int total = 0;
    for (const auto& q : transfer_queue_) {
        if (q.status == QueuedTransferStatus::QUEUED || q.status == QueuedTransferStatus::ACTIVE) {
            total++;
        }
    }

    int position = 1;
    for (const auto& q : transfer_queue_) {
        if (q.status == QueuedTransferStatus::QUEUED || q.status == QueuedTransferStatus::ACTIVE) {
            queue_status_callback_(q.queue_id, position, total);
            position++;
        }
    }
}

void TransferManager::cleanup_old_queue_entries() {
    // Keep only the last 10 completed/failed entries
    const size_t max_history = 10;

    // Count completed/failed entries
    size_t completed_count = 0;
    for (const auto& q : transfer_queue_) {
        if (q.status == QueuedTransferStatus::COMPLETED ||
            q.status == QueuedTransferStatus::FAILED ||
            q.status == QueuedTransferStatus::CANCELLED) {
            completed_count++;
        }
    }

    // Remove oldest completed entries if we have too many
    while (completed_count > max_history) {
        for (auto it = transfer_queue_.begin(); it != transfer_queue_.end(); ++it) {
            if (it->status == QueuedTransferStatus::COMPLETED ||
                it->status == QueuedTransferStatus::FAILED ||
                it->status == QueuedTransferStatus::CANCELLED) {
                transfer_queue_.erase(it);
                completed_count--;
                break;
            }
        }
    }
}

void TransferManager::execute_send_transfer(const std::string& transfer_id) {
    LOG_CORE_INFO() << "Starting send execution for transfer " << transfer_id;

    // Get transfer info
    TransferInfo transfer;
    std::vector<std::string> file_paths;
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        auto it = active_transfers_.find(transfer_id);
        if (it == active_transfers_.end()) {
            LOG_CORE_ERROR() << "Transfer not found: " << transfer_id;
            return;
        }
        transfer = it->second;

        auto paths_it = send_file_paths_.find(transfer_id);
        if (paths_it == send_file_paths_.end()) {
            LOG_CORE_ERROR() << "File paths not found for transfer: " << transfer_id;
            return;
        }
        file_paths = paths_it->second;
    }

    // Check dependencies
    if (!api_client_) {
        LOG_CORE_ERROR() << "API client not set";
        if (completion_callback_) {
            completion_callback_(transfer_id, false, "Internal error: API client not configured");
        }
        return;
    }

    if (!peer_lookup_callback_) {
        LOG_CORE_ERROR() << "Peer lookup callback not set";
        if (completion_callback_) {
            completion_callback_(transfer_id, false, "Internal error: Peer lookup not configured");
        }
        return;
    }

    // Look up peer connection info
    PeerConnectionInfo peer_info = peer_lookup_callback_(transfer.peer_device_id);
    if (peer_info.host.empty() || peer_info.port <= 0) {
        LOG_CORE_ERROR() << "Could not find peer: " << transfer.peer_device_id;
        if (completion_callback_) {
            completion_callback_(transfer_id, false, "Peer not found or offline");
        }
        return;
    }

    LOG_CORE_INFO() << "Sending to peer at " << peer_info.host << ":" << peer_info.port;

    // Build transfer request
    TransferRequest request;
    request.files = transfer.files;

    // Step 1: Request transfer approval from peer
    LOG_CORE_INFO() << "Requesting transfer approval...";
    auto response = api_client_->request_transfer(peer_info.host, peer_info.port,
                                                   peer_info.fingerprint, request);
    if (!response.success) {
        LOG_CORE_ERROR() << "Transfer request failed: " << response.error_message;
        if (completion_callback_) {
            completion_callback_(transfer_id, false, "Transfer request rejected: " + response.error_message);
        }
        return;
    }

    LOG_CORE_INFO() << "Transfer approved, starting file upload...";

    // Step 2: Upload each file
    uint64_t total_transferred = 0;
    const size_t chunk_size = 1024 * 1024;  // 1MB chunks

    for (size_t file_index = 0; file_index < file_paths.size(); ++file_index) {
        const std::string& file_path = file_paths[file_index];
        const FileMetadata& file_meta = transfer.files[file_index];

        LOG_CORE_INFO() << "Uploading file " << (file_index + 1) << "/" << file_paths.size()
                        << ": " << file_meta.name << " (" << file_meta.size << " bytes)";

        // Read file in chunks and upload
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            LOG_CORE_ERROR() << "Failed to open file: " << file_path;
            if (completion_callback_) {
                completion_callback_(transfer_id, false, "Failed to read file: " + file_meta.name);
            }
            return;
        }

        std::vector<uint8_t> buffer(chunk_size);
        uint64_t file_transferred = 0;

        while (file_transferred < file_meta.size) {
            size_t to_read = std::min(chunk_size, static_cast<size_t>(file_meta.size - file_transferred));
            file.read(reinterpret_cast<char*>(buffer.data()), to_read);
            size_t actually_read = file.gcount();

            if (actually_read == 0) {
                break;  // EOF
            }

            // Resize buffer to actual read size
            std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + actually_read);

            // Upload chunk
            auto upload_response = api_client_->upload_file(
                peer_info.host, peer_info.port, peer_info.fingerprint,
                transfer_id, static_cast<int>(file_index), chunk);

            if (!upload_response.success) {
                LOG_CORE_ERROR() << "File upload failed: " << upload_response.error_message;
                if (completion_callback_) {
                    completion_callback_(transfer_id, false, "Upload failed: " + upload_response.error_message);
                }
                return;
            }

            file_transferred += actually_read;
            total_transferred += actually_read;

            // Update progress
            {
                std::lock_guard<std::mutex> lock(transfers_mutex_);
                auto it = active_transfers_.find(transfer_id);
                if (it != active_transfers_.end()) {
                    it->second.transferred_bytes = total_transferred;
                }
            }

            if (progress_callback_) {
                float progress = transfer.total_bytes > 0 ?
                    (static_cast<float>(total_transferred) / transfer.total_bytes) * 100.0f : 0.0f;
                progress_callback_(transfer_id, progress, total_transferred);
            }
        }

        file.close();
        LOG_CORE_INFO() << "Completed file: " << file_meta.name;
    }

    // Mark as completed
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        auto it = active_transfers_.find(transfer_id);
        if (it != active_transfers_.end()) {
            it->second.status = TransferStatus::COMPLETED;
        }
        send_file_paths_.erase(transfer_id);
    }

    LOG_CORE_INFO() << "Transfer completed successfully: " << transfer_id;
    if (completion_callback_) {
        completion_callback_(transfer_id, true, "");
    }
}

} // namespace warpdeck