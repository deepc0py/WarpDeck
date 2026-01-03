#ifndef WARPDECK_H
#define WARPDECK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*
 * WarpDeck C API
 *
 * Memory Ownership Rules:
 * -----------------------
 * Functions that return `const char*` fall into two categories:
 *
 * 1. CALLER-OWNED (must free with warpdeck_free_string):
 *    - warpdeck_get_trusted_devices()
 *    - warpdeck_get_discovery_status()
 *    - warpdeck_get_discovered_peers()
 *    - warpdeck_get_mdns_debug_info()
 *
 * 2. CALLBACK strings (must free with warpdeck_free_string after processing):
 *    - All callback string parameters are heap-allocated
 *    - The callback recipient owns the memory and must free it
 *
 * Example:
 *   const char* status = warpdeck_get_discovery_status(handle);
 *   if (status) {
 *       // use status...
 *       warpdeck_free_string(status);  // REQUIRED
 *   }
 */

// Forward declaration for opaque handle
typedef struct WarpDeckHandle WarpDeckHandle;

// Callback function types
// NOTE: All string parameters in callbacks are heap-allocated.
// The callback recipient MUST call warpdeck_free_string() after processing.
typedef void (*on_peer_discovered_callback)(const char* peer_json);
typedef void (*on_peer_lost_callback)(const char* device_id);
typedef void (*on_incoming_transfer_request_callback)(const char* transfer_request_json);
typedef void (*on_transfer_progress_update_callback)(const char* transfer_id, float progress_percent, uint64_t bytes_transferred);
typedef void (*on_transfer_completed_callback)(const char* transfer_id, bool success, const char* error_message);
typedef void (*on_error_callback)(const char* error_message);

// Callbacks struct containing all event callbacks
typedef struct {
    on_peer_discovered_callback on_peer_discovered;
    on_peer_lost_callback on_peer_lost;
    on_incoming_transfer_request_callback on_incoming_transfer_request;
    on_transfer_progress_update_callback on_transfer_progress_update;
    on_transfer_completed_callback on_transfer_completed;
    on_error_callback on_error;
} Callbacks;

// ============================================================================
// Core library functions
// ============================================================================

// Create a new WarpDeck instance. Returns NULL on failure.
WarpDeckHandle* warpdeck_create(const Callbacks* callbacks, const char* config_dir);

// Destroy a WarpDeck instance and free all resources.
void warpdeck_destroy(WarpDeckHandle* handle);

// Start the WarpDeck service. Returns port number on success, negative on error.
int warpdeck_start(WarpDeckHandle* handle, const char* device_name, int desired_port);

// Stop the WarpDeck service.
void warpdeck_stop(WarpDeckHandle* handle);

// Update the device name used for discovery.
void warpdeck_set_device_name(WarpDeckHandle* handle, const char* new_name);

// Initiate a file transfer to a peer.
void warpdeck_initiate_transfer(WarpDeckHandle* handle, const char* device_id, const char* files_json);

// Accept or reject an incoming transfer request.
void warpdeck_respond_to_transfer(WarpDeckHandle* handle, const char* transfer_id, bool accept);

// Cancel an in-progress transfer.
void warpdeck_cancel_transfer(WarpDeckHandle* handle, const char* transfer_id);

// Get list of trusted devices as JSON.
// CALLER OWNS: Must call warpdeck_free_string() on returned value.
const char* warpdeck_get_trusted_devices(WarpDeckHandle* handle);

// Remove a device from the trust store.
void warpdeck_remove_trusted_device(WarpDeckHandle* handle, const char* device_id);

// ============================================================================
// Transfer Queue functions
// ============================================================================

// Queue a transfer instead of starting immediately.
// Transfers are processed sequentially (one at a time).
// Returns queue_id on success, NULL on failure.
// CALLER OWNS: Must call warpdeck_free_string() on returned value.
const char* warpdeck_queue_transfer(WarpDeckHandle* handle, const char* device_id, const char* files_json);

// Cancel a queued or active transfer by queue_id.
// Returns true if cancelled, false if not found.
bool warpdeck_cancel_queued_transfer(WarpDeckHandle* handle, const char* queue_id);

// Get current queue status as JSON array.
// Format: [{"queueId": "...", "peerName": "...", "status": 0-4, "fileCount": N, "totalBytes": N}, ...]
// Status: 0=QUEUED, 1=ACTIVE, 2=COMPLETED, 3=FAILED, 4=CANCELLED
// CALLER OWNS: Must call warpdeck_free_string() on returned value.
const char* warpdeck_get_queue_status(WarpDeckHandle* handle);

// ============================================================================
// Debug and diagnostic functions
// ============================================================================

// Get current discovery status as JSON.
// CALLER OWNS: Must call warpdeck_free_string() on returned value.
const char* warpdeck_get_discovery_status(WarpDeckHandle* handle);

// Get list of discovered peers as JSON.
// CALLER OWNS: Must call warpdeck_free_string() on returned value.
const char* warpdeck_get_discovered_peers(WarpDeckHandle* handle);

// Get mDNS debug information as a string.
// CALLER OWNS: Must call warpdeck_free_string() on returned value.
const char* warpdeck_get_mdns_debug_info(WarpDeckHandle* handle);

// ============================================================================
// Memory management
// ============================================================================

// Free a string returned by the library.
// Safe to call with NULL.
void warpdeck_free_string(const char* str);

#ifdef __cplusplus
}
#endif

#endif // WARPDECK_H