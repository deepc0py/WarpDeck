// Generated FFI bindings for libwarpdeck
// ignore_for_file: non_constant_identifier_names, unused_import, camel_case_types

import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:path/path.dart' as path;

// --- Start of Manually Adjusted Singleton Pattern ---

class WarpDeckFFI {
  static WarpDeckFFI? _instance;
  static WarpDeckFFI get instance {
    _instance ??= WarpDeckFFI._();
    return _instance!;
  }

  late final ffi.DynamicLibrary _lib;

  WarpDeckFFI._() {
    _lib = _loadLibrary();
    _bindFunctions();
  }

  ffi.DynamicLibrary _loadLibrary() {
    // Check for environment variable override (useful for development)
    final envPath = Platform.environment['LIBWARPDECK_PATH'];
    if (envPath != null && envPath.isNotEmpty) {
      try {
        print('Loading library from LIBWARPDECK_PATH: $envPath');
        return ffi.DynamicLibrary.open(envPath);
      } catch (e) {
        print('Failed to load from LIBWARPDECK_PATH: $e');
      }
    }

    if (Platform.isMacOS) {
      final executableDir = path.dirname(Platform.resolvedExecutable);
      final possiblePaths = [
        // Production: Inside app bundle
        path.join(executableDir, 'libwarpdeck.dylib'),
        path.join(executableDir, '..', 'Frameworks', 'libwarpdeck.dylib'),
        // Development: Relative to project structure (flutter run from warpdeck_gui)
        '../../../libwarpdeck/build/libwarpdeck.dylib',
        // Fallback: Current directory or system path
        'libwarpdeck.dylib',
      ];
      print('Executable directory: $executableDir');
      for (final p in possiblePaths) {
        try {
          print('Trying to load library from: $p');
          final lib = ffi.DynamicLibrary.open(p);
          print('Successfully loaded library from: $p');
          return lib;
        } catch (e) {
          print('Failed to load from $p: $e');
        }
      }
    }
    if (Platform.isLinux) {
      final executableDir = path.dirname(Platform.resolvedExecutable);
      final possiblePaths = [
        // Production: Inside app bundle
        path.join(executableDir, 'lib', 'libwarpdeck.so'),
        path.join(executableDir, 'libwarpdeck.so'),
        // Development: Relative to project structure
        '../../../libwarpdeck/build/libwarpdeck.so',
        // Fallback: Current directory or system path
        'libwarpdeck.so',
      ];
      for (final p in possiblePaths) {
        try {
          print('Trying to load library from: $p');
          final lib = ffi.DynamicLibrary.open(p);
          print('Successfully loaded library from: $p');
          return lib;
        } catch (e) {
          print('Failed to load from $p: $e');
        }
      }
    }
    throw Exception(
        'Failed to load libwarpdeck dynamic library. '
        'Set LIBWARPDECK_PATH environment variable for custom location.');
  }

// --- End of Manually Adjusted Singleton Pattern ---

// --- Auto-generated FFI function bindings ---

late final warpdeck_create_dart warpdeckCreate;
late final warpdeck_destroy_dart warpdeckDestroy;
late final warpdeck_start_dart warpdeckStart;
late final warpdeck_stop_dart warpdeckStop;
late final warpdeck_set_device_name_dart warpdeckSetDeviceName;
late final warpdeck_initiate_transfer_dart warpdeckInitiateTransfer;
late final warpdeck_respond_to_transfer_dart warpdeckRespondToTransfer;
late final warpdeck_cancel_transfer_dart warpdeckCancelTransfer;
late final warpdeck_get_trusted_devices_dart warpdeckGetTrustedDevices;
late final warpdeck_remove_trusted_device_dart warpdeckRemoveTrustedDevice;
late final warpdeck_get_discovery_status_dart warpdeckGetDiscoveryStatus;
late final warpdeck_get_discovered_peers_dart warpdeckGetDiscoveredPeers;
late final warpdeck_get_mdns_debug_info_dart warpdeckGetMdnsDebugInfo;
late final warpdeck_free_string_dart warpdeckFreeString;

// Queue functions
late final warpdeck_queue_transfer_dart warpdeckQueueTransfer;
late final warpdeck_cancel_queued_transfer_dart warpdeckCancelQueuedTransfer;
late final warpdeck_get_queue_status_dart warpdeckGetQueueStatus;

void _bindFunctions() {
    warpdeckCreate = _lib.lookupFunction<warpdeck_create_native, warpdeck_create_dart>('warpdeck_create');
    warpdeckDestroy = _lib.lookupFunction<warpdeck_destroy_native, warpdeck_destroy_dart>('warpdeck_destroy');
    warpdeckStart = _lib.lookupFunction<warpdeck_start_native, warpdeck_start_dart>('warpdeck_start');
    warpdeckStop = _lib.lookupFunction<warpdeck_stop_native, warpdeck_stop_dart>('warpdeck_stop');
    warpdeckSetDeviceName = _lib.lookupFunction<warpdeck_set_device_name_native, warpdeck_set_device_name_dart>('warpdeck_set_device_name');
    warpdeckInitiateTransfer = _lib.lookupFunction<warpdeck_initiate_transfer_native, warpdeck_initiate_transfer_dart>('warpdeck_initiate_transfer');
    warpdeckRespondToTransfer = _lib.lookupFunction<warpdeck_respond_to_transfer_native, warpdeck_respond_to_transfer_dart>('warpdeck_respond_to_transfer');
    warpdeckCancelTransfer = _lib.lookupFunction<warpdeck_cancel_transfer_native, warpdeck_cancel_transfer_dart>('warpdeck_cancel_transfer');
    warpdeckGetTrustedDevices = _lib.lookupFunction<warpdeck_get_trusted_devices_native, warpdeck_get_trusted_devices_dart>('warpdeck_get_trusted_devices');
    warpdeckRemoveTrustedDevice = _lib.lookupFunction<warpdeck_remove_trusted_device_native, warpdeck_remove_trusted_device_dart>('warpdeck_remove_trusted_device');
    warpdeckGetDiscoveryStatus = _lib.lookupFunction<warpdeck_get_discovery_status_native, warpdeck_get_discovery_status_dart>('warpdeck_get_discovery_status');
    warpdeckGetDiscoveredPeers = _lib.lookupFunction<warpdeck_get_discovered_peers_native, warpdeck_get_discovered_peers_dart>('warpdeck_get_discovered_peers');
    warpdeckGetMdnsDebugInfo = _lib.lookupFunction<warpdeck_get_mdns_debug_info_native, warpdeck_get_mdns_debug_info_dart>('warpdeck_get_mdns_debug_info');
    warpdeckFreeString = _lib.lookupFunction<warpdeck_free_string_native, warpdeck_free_string_dart>('warpdeck_free_string');

    // Queue functions
    warpdeckQueueTransfer = _lib.lookupFunction<warpdeck_queue_transfer_native, warpdeck_queue_transfer_dart>('warpdeck_queue_transfer');
    warpdeckCancelQueuedTransfer = _lib.lookupFunction<warpdeck_cancel_queued_transfer_native, warpdeck_cancel_queued_transfer_dart>('warpdeck_cancel_queued_transfer');
    warpdeckGetQueueStatus = _lib.lookupFunction<warpdeck_get_queue_status_native, warpdeck_get_queue_status_dart>('warpdeck_get_queue_status');
  }
}

// --- Native and Dart typedefs ---

typedef on_peer_discovered_native = ffi.Void Function(ffi.Pointer<ffi.Char> peer_json);
typedef on_peer_lost_native = ffi.Void Function(ffi.Pointer<ffi.Char> device_id);
typedef on_incoming_transfer_request_native = ffi.Void Function(ffi.Pointer<ffi.Char> transfer_request_json);
typedef on_transfer_progress_update_native = ffi.Void Function(ffi.Pointer<ffi.Char> transfer_id, ffi.Float progress_percent, ffi.Uint64 bytes_transferred);
typedef on_transfer_completed_native = ffi.Void Function(ffi.Pointer<ffi.Char> transfer_id, ffi.Bool success, ffi.Pointer<ffi.Char> error_message);
typedef on_error_native = ffi.Void Function(ffi.Pointer<ffi.Char> error_message);

typedef on_peer_discovered_dart = void Function(ffi.Pointer<ffi.Char> peer_json);
typedef on_peer_lost_dart = void Function(ffi.Pointer<ffi.Char> device_id);
typedef on_incoming_transfer_request_dart = void Function(ffi.Pointer<ffi.Char> transfer_request_json);
typedef on_transfer_progress_update_dart = void Function(ffi.Pointer<ffi.Char> transfer_id, double progress_percent, int bytes_transferred);
typedef on_transfer_completed_dart = void Function(ffi.Pointer<ffi.Char> transfer_id, bool success, ffi.Pointer<ffi.Char> error_message);
typedef on_error_dart = void Function(ffi.Pointer<ffi.Char> error_message);

final class Callbacks extends ffi.Struct {
  external ffi.Pointer<ffi.NativeFunction<on_peer_discovered_native>> on_peer_discovered;
  external ffi.Pointer<ffi.NativeFunction<on_peer_lost_native>> on_peer_lost;
  external ffi.Pointer<ffi.NativeFunction<on_incoming_transfer_request_native>> on_incoming_transfer_request;
  external ffi.Pointer<ffi.NativeFunction<on_transfer_progress_update_native>> on_transfer_progress_update;
  external ffi.Pointer<ffi.NativeFunction<on_transfer_completed_native>> on_transfer_completed;
  external ffi.Pointer<ffi.NativeFunction<on_error_native>> on_error;
}

final class WarpDeckHandle extends ffi.Opaque {}

// Function typedefs
typedef warpdeck_create_native = ffi.Pointer<WarpDeckHandle> Function(ffi.Pointer<Callbacks> callbacks, ffi.Pointer<ffi.Char> config_dir);
typedef warpdeck_create_dart = ffi.Pointer<WarpDeckHandle> Function(ffi.Pointer<Callbacks> callbacks, ffi.Pointer<ffi.Char> config_dir);

typedef warpdeck_destroy_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_destroy_dart = void Function(ffi.Pointer<WarpDeckHandle> handle);

typedef warpdeck_start_native = ffi.Int Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_name, ffi.Int desired_port);
typedef warpdeck_start_dart = int Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_name, int desired_port);

typedef warpdeck_stop_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_stop_dart = void Function(ffi.Pointer<WarpDeckHandle> handle);

typedef warpdeck_set_device_name_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> new_name);
typedef warpdeck_set_device_name_dart = void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> new_name);

typedef warpdeck_initiate_transfer_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_id, ffi.Pointer<ffi.Char> files_json);
typedef warpdeck_initiate_transfer_dart = void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_id, ffi.Pointer<ffi.Char> files_json);

typedef warpdeck_respond_to_transfer_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> transfer_id, ffi.Bool accept);
typedef warpdeck_respond_to_transfer_dart = void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> transfer_id, bool accept);

typedef warpdeck_cancel_transfer_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> transfer_id);
typedef warpdeck_cancel_transfer_dart = void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> transfer_id);

typedef warpdeck_get_trusted_devices_native = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_get_trusted_devices_dart = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);

typedef warpdeck_remove_trusted_device_native = ffi.Void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_id);
typedef warpdeck_remove_trusted_device_dart = void Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_id);

typedef warpdeck_get_discovery_status_native = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_get_discovery_status_dart = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);

typedef warpdeck_get_discovered_peers_native = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_get_discovered_peers_dart = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);

typedef warpdeck_get_mdns_debug_info_native = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_get_mdns_debug_info_dart = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);

typedef warpdeck_free_string_native = ffi.Void Function(ffi.Pointer<ffi.Char> str);
typedef warpdeck_free_string_dart = void Function(ffi.Pointer<ffi.Char> str);

// Queue function typedefs
typedef warpdeck_queue_transfer_native = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_id, ffi.Pointer<ffi.Char> files_json);
typedef warpdeck_queue_transfer_dart = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> device_id, ffi.Pointer<ffi.Char> files_json);

typedef warpdeck_cancel_queued_transfer_native = ffi.Bool Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> queue_id);
typedef warpdeck_cancel_queued_transfer_dart = bool Function(ffi.Pointer<WarpDeckHandle> handle, ffi.Pointer<ffi.Char> queue_id);

typedef warpdeck_get_queue_status_native = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);
typedef warpdeck_get_queue_status_dart = ffi.Pointer<ffi.Char> Function(ffi.Pointer<WarpDeckHandle> handle);