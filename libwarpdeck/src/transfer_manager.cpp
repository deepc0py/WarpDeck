#include "transfer_manager.h"
#include "utils.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace warpdeck {

TransferManager::TransferManager() {
    download_folder_ = utils::get_default_download_dir();
}

TransferManager::~TransferManager() {}

void TransferManager::set_download_folder(const std::string& folder) {
    download_folder_ = folder;
}

void TransferManager::set_progress_callback(ProgressCallback callback) {
    progress_callback_ = callback;
}

void TransferManager::set_completion_callback(CompletionCallback callback) {
    completion_callback_ = callback;
}

void TransferManager::set_incoming_request_callback(IncomingRequestCallback callback) {
    incoming_request_callback_ = callback;
}

std::string TransferManager::initiate_transfer(const std::string& peer_device_id, const std::string& peer_name,
                                              const std::vector<std::string>& file_paths) {
    std::string transfer_id = generate_transfer_id();
    
    TransferInfo transfer;
    transfer.transfer_id = transfer_id;
    transfer.peer_device_id = peer_device_id;
    transfer.peer_name = peer_name;
    transfer.direction = TransferDirection::SENDING;
    transfer.status = TransferStatus::PENDING_APPROVAL;
    transfer.total_bytes = 0;
    transfer.transferred_bytes = 0;
    
    // Build file metadata
    for (const auto& file_path : file_paths) {
        if (!utils::file_exists(file_path)) {
            continue;
        }
        
        FileMetadata file_meta;
        file_meta.name = utils::get_filename(file_path);
        file_meta.size = utils::get_file_size(file_path);
        file_meta.hash = utils::calculate_file_hash(file_path);
        
        transfer.files.push_back(file_meta);
        transfer.total_bytes += file_meta.size;
    }
    
    if (transfer.files.empty()) {
        return ""; // No valid files
    }
    
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        active_transfers_[transfer_id] = transfer;
    }
    
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
                    std::string final_path = transfer.destination_folder + "/" + transfer.files[i].name;
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
    std::string final_path = transfer_it->second.destination_folder + "/" + file_meta.name;
    
    try {
        // Ensure destination directory exists
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

} // namespace warpdeck