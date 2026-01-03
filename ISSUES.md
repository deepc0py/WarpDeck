# WarpDeck Code Quality Issues

Identified: 2025-12-29

## Critical Issues (Must Fix)

- [x] **1. Memory mismatch: strdup/delete[]** - `libwarpdeck/src/warpdeck.cpp:86-128`
  - Callbacks allocate with `strdup()` (malloc), but Dart frees with `delete[]` (C++ new)
  - Undefined behavior, will cause memory corruption
  - Fix: Use `new char[]` + `delete[]` consistently, or `malloc` + `free`
  - **FIXED**: Replaced all `strdup()` calls with `copy_string()` which uses `new char[]`

- [x] **2. Static race condition** - `libwarpdeck/src/mdns_manager.cpp:504`
  - `static std::map<std::string, PeerInfo> pending_peers` persists across instances
  - Not thread-safe, pollutes state between instances
  - Fix: Move to class member variable
  - **FIXED**: Moved `pending_peers_` and `pending_peers_mutex_` to class member variables

- [x] **3. Detached thread without lifecycle** - `libwarpdeck/src/api_server.cpp:59-67`
  - Server thread detached without synchronization
  - Crash risk when stopping server while thread accesses freed memory
  - Fix: Use `join()` with proper shutdown sequence
  - **FIXED**: Added `server_thread_` member, use `join()` in `stop()`

- [x] **4. Hardcoded "unknown_peer"** - `libwarpdeck/src/warpdeck.cpp:147`
  - Incoming transfers use placeholder instead of extracting peer identity from TLS cert
  - Breaks security model (trust based on fingerprint but using placeholder)
  - Fix: Extract peer device_id from TLS session
  - **FIXED**: Now looks up peer by fingerprint in discovered peers, falls back to fingerprint as ID

- [x] **5. Buffer overflow in mDNS parsing** - `libwarpdeck/src/mdns_manager.cpp:540-544`
  - No validation that `record_length >= 6` before reading SRV record bytes
  - Malformed packets cause buffer overflow
  - Fix: Add bounds check
  - **ALREADY FIXED**: Bounds check exists at line 540, caller also validates bounds

## High Priority Issues

- [x] **6. Raw pointer leaks** - `libwarpdeck/src/warpdeck.cpp:350-413`
  - `copy_string()` uses `new char[]` requiring manual `warpdeck_free_string()`
  - Functions: `warpdeck_get_discovery_status()`, `warpdeck_get_discovered_peers()`, `warpdeck_get_mdns_debug_info()`
  - Fix: Document ownership clearly or use RAII
  - **FIXED**: Added comprehensive documentation to warpdeck.h header describing memory ownership rules

- [x] **7. Magic numbers everywhere** - Multiple files
  - Ports (5353, 54321, 65535), TTLs (300, 4500, 120), service name "_warpdeck._tcp.local."
  - Repeated in 10+ locations
  - Fix: Create `constants.h`
  - **FIXED**: Created `libwarpdeck/src/constants.h`, updated api_server.cpp and mdns_manager.cpp

- [x] **8. Missing const-correctness** - Headers
  - Methods that don't modify state aren't marked `const`
  - Example: `get_discovered_peers()` should be const
  - Fix: Add const to appropriate methods
  - **FIXED**: Verified headers already have good const-correctness (`get_discovered_peers() const`, etc.)

- [x] **9. Long complex functions** - `libwarpdeck/src/mdns_manager.cpp`
  - `process_mdns_response()` is 170+ lines
  - Fix: Extract `parse_ptr_record()`, `parse_srv_record()`, `parse_txt_record()`
  - **FIXED**: Refactored into 5 helper functions: `parse_ptr_record()`, `parse_srv_record()`, `parse_txt_record()`, `merge_pending_peer()`, `finalize_discovered_peer()`

- [x] **10. Inconsistent JSON building** - `libwarpdeck/src/utils.cpp`
  - Manual JSON building, mixed camelCase/snake_case
  - Fix: Use single adapter/serializer pattern
  - **FIXED**: Converted all JSON to use nlohmann::json consistently with camelCase field names matching Dart models

## Flutter/Dart Issues

- [x] **11. Static service instance** - `warpdeck_service.dart:66`
  - `static WarpDeckService? _instance` prevents garbage collection
  - Fix: Explicit cleanup or WeakReference
  - **NOT A BUG**: Required by Dart FFI - NativeCallable callbacks must be static, need instance reference

- [x] **12. NativeCallable listeners never disposed** - `warpdeck_service.dart:123-128`
  - 6 NativeCallables allocated but never disposed
  - Fix: Implement disposal in StateNotifier lifecycle
  - **ALREADY FIXED**: `dispose()` method (lines 282-288) properly closes all listeners

- [x] **13. Hardcoded development path** - `libwarpdeck_ffi.dart:31`
  - `/Users/jesse/code/WarpDeck/libwarpdeck/build/libwarpdeck.dylib`
  - Won't work on other machines
  - Fix: Use app bundle resources or platform directories
  - **FIXED**: Removed hardcoded path, added LIBWARPDECK_PATH env var support, improved path search order

- [ ] **14. State watching causes over-rebuilds** - `dashboard_screen.dart`, `peer_list_widget.dart`
  - `ref.watch(warpDeckServiceProvider)` watches entire state
  - Fix: Use `select()` to watch only needed fields

- [x] **15. No error boundary for FFI failures** - `main.dart`
  - If FFI fails, entire app crashes
  - Fix: Wrap initialization in try-catch with fallback UI
  - **ALREADY FIXED**: `warpdeck_service.dart:111-120` catches FFI errors, `StatusIndicator` shows error dialog

- [ ] **16. Code duplication in UI** - Multiple files
  - Time formatting repeated, peer details dialog inline (32 lines)
  - Fix: Extract utility functions and reusable widgets

- [x] **17. Missing async timeout protection** - `warpdeck_service.dart`
  - Only `warpdeck_start()` has timeout; other FFI calls can hang
  - Fix: Wrap all FFI calls with timeouts
  - **LOW RISK**: Only `start()` can hang (network); other calls are synchronous queries/commands

- [x] **18. Callback design not thread-safe** - Various callbacks
  - C++ callbacks from network threads call Dart synchronously
  - Fix: Queue events to main thread
  - **ALREADY FIXED**: Uses `NativeCallable.listener` which safely posts to Dart isolate's event loop

## Architecture Issues

- [ ] **19. Inconsistent error handling** - Multiple files
  - `warpdeck_start()` returns port/-1/-2, others return bool
  - Fix: Define error code enum or Result<T> type

- [x] **20. File path validation missing** - `transfer_manager.cpp`
  - No sanitization for path traversal (`../../etc`)
  - Security risk: arbitrary file write
  - Fix: Validate all paths, reject relative paths
  - **FIXED**: Added `utils::sanitize_filename()` that strips path components and control chars

- [ ] **21. Logger singleton anti-pattern** - `logger.h/cpp`
  - Global `Logger::instance()` throughout code
  - Fix: Dependency injection

- [ ] **22. No state machine for discovery** - Multiple files
  - Complex state transitions scattered
  - Fix: Explicit state machine class

- [ ] **23. Repeated model boilerplate** - `peer.dart`, `transfer.dart`
  - Manual `copyWith()` for every field
  - Fix: Use Freezed package

- [ ] **24. Manual socket management** - `mdns_manager.cpp`
  - Raw socket vectors, manual creation/cleanup
  - Fix: RAII socket manager class

---

## Progress

### Completed (2025-12-29)
**Critical Issues (5/5)**
- [x] 1. Memory mismatch: strdup/delete[]
- [x] 2. Static race condition in pending_peers
- [x] 3. Detached thread lifecycle
- [x] 4. Hardcoded unknown_peer
- [x] 5. Buffer overflow in mDNS (already had bounds check)

**High Priority Issues (5/5)**
- [x] 6. Raw pointer leaks - documented ownership in warpdeck.h
- [x] 7. Magic numbers (created constants.h)
- [x] 8. Const-correctness (verified already present)
- [x] 9. Long complex functions (refactored process_mdns_response)
- [x] 10. Inconsistent JSON building (standardized to nlohmann::json with camelCase)
- [x] 13. Hardcoded development path in FFI

**Flutter/Dart Issues (6/8 - 2 deferred)**
- [x] 11. Static service instance (NOT A BUG - required by Dart FFI)
- [x] 12. NativeCallable listeners (ALREADY FIXED in dispose())
- [x] 15. No error boundary (ALREADY FIXED - StatusIndicator shows errors)
- [x] 17. Async timeout protection (LOW RISK - only start() can hang)
- [x] 18. Callback thread safety (ALREADY FIXED - uses NativeCallable.listener)
- [ ] 14. State watching over-rebuilds (optimization - deferred)
- [ ] 16. Code duplication in UI (minor - deferred)

**Architecture Issues (1/6)**
- [x] 20. File path validation (SECURITY FIX - added sanitize_filename())

### Remaining (Low Priority)
- [ ] 14. State watching over-rebuilds (optimization)
- [ ] 16. Code duplication in UI (minor refactor)
- [ ] 19. Inconsistent error handling (code style)
- [ ] 21. Logger singleton (architecture preference)
- [ ] 22. No state machine for discovery (architecture preference)
- [ ] 23. Repeated model boilerplate (use Freezed - style preference)
- [ ] 24. Manual socket management (RAII - code style)
