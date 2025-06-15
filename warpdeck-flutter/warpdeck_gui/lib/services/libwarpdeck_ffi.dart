import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as path;

// Typedefs for libwarpdeck C API
typedef WarpDeckHandle = Opaque;

// Callback function typedefs
typedef OnPeerDiscoveredNative = Void Function(Pointer<Utf8> peerJson);
typedef OnPeerLostNative = Void Function(Pointer<Utf8> deviceId);
typedef OnIncomingTransferRequestNative = Void Function(Pointer<Utf8> transferRequestJson);
typedef OnTransferProgressUpdateNative = Void Function(Pointer<Utf8> transferId, Float progress, Uint64 bytesTransferred);
typedef OnTransferCompletedNative = Void Function(Pointer<Utf8> transferId, Bool success, Pointer<Utf8> errorMessage);
typedef OnErrorNative = Void Function(Pointer<Utf8> errorMessage);

typedef OnPeerDiscoveredDart = void Function(Pointer<Utf8> peerJson);
typedef OnPeerLostDart = void Function(Pointer<Utf8> deviceId);
typedef OnIncomingTransferRequestDart = void Function(Pointer<Utf8> transferRequestJson);
typedef OnTransferProgressUpdateDart = void Function(Pointer<Utf8> transferId, double progress, int bytesTransferred);
typedef OnTransferCompletedDart = void Function(Pointer<Utf8> transferId, bool success, Pointer<Utf8> errorMessage);
typedef OnErrorDart = void Function(Pointer<Utf8> errorMessage);

// Callbacks struct
final class Callbacks extends Struct {
  external Pointer<NativeFunction<OnPeerDiscoveredNative>> onPeerDiscovered;
  external Pointer<NativeFunction<OnPeerLostNative>> onPeerLost;
  external Pointer<NativeFunction<OnIncomingTransferRequestNative>> onIncomingTransferRequest;
  external Pointer<NativeFunction<OnTransferProgressUpdateNative>> onTransferProgressUpdate;
  external Pointer<NativeFunction<OnTransferCompletedNative>> onTransferCompleted;
  external Pointer<NativeFunction<OnErrorNative>> onError;
}

// libwarpdeck API functions
typedef WarpDeckCreateNative = Pointer<WarpDeckHandle> Function(Pointer<Callbacks> callbacks, Pointer<Utf8> configDir);
typedef WarpDeckCreateDart = Pointer<WarpDeckHandle> Function(Pointer<Callbacks> callbacks, Pointer<Utf8> configDir);

typedef WarpDeckStartNative = Int32 Function(Pointer<WarpDeckHandle> handle, Pointer<Utf8> deviceName, Int32 desiredPort);
typedef WarpDeckStartDart = int Function(Pointer<WarpDeckHandle> handle, Pointer<Utf8> deviceName, int desiredPort);

typedef WarpDeckStopNative = Void Function(Pointer<WarpDeckHandle> handle);
typedef WarpDeckStopDart = void Function(Pointer<WarpDeckHandle> handle);

typedef WarpDeckDestroyNative = Void Function(Pointer<WarpDeckHandle> handle);
typedef WarpDeckDestroyDart = void Function(Pointer<WarpDeckHandle> handle);

typedef WarpDeckInitiateTransferNative = Void Function(Pointer<WarpDeckHandle> handle, Pointer<Utf8> targetId, Pointer<Utf8> filesJson);
typedef WarpDeckInitiateTransferDart = void Function(Pointer<WarpDeckHandle> handle, Pointer<Utf8> targetId, Pointer<Utf8> filesJson);

typedef WarpDeckRespondToTransferNative = Void Function(Pointer<WarpDeckHandle> handle, Pointer<Utf8> transferId, Bool accepted);
typedef WarpDeckRespondToTransferDart = void Function(Pointer<WarpDeckHandle> handle, Pointer<Utf8> transferId, bool accepted);

class WarpDeckFFI {
  static WarpDeckFFI? _instance;
  static WarpDeckFFI get instance => _instance ??= WarpDeckFFI._();
  
  late final DynamicLibrary _lib;
  late final WarpDeckCreateDart warpdeckCreate;
  late final WarpDeckStartDart warpdeckStart;
  late final WarpDeckStopDart warpdeckStop;
  late final WarpDeckDestroyDart warpdeckDestroy;
  late final WarpDeckInitiateTransferDart warpdeckInitiateTransfer;
  late final WarpDeckRespondToTransferDart warpdeckRespondToTransfer;

  WarpDeckFFI._() {
    _loadLibrary();
    _bindFunctions();
  }

  void _loadLibrary() {
    // Load the libwarpdeck library
    try {
      if (Platform.isMacOS) {
        // Try multiple paths for the dylib
        final executableDir = path.dirname(Platform.resolvedExecutable);
        final possiblePaths = [
          // Bundled with app in same directory as executable
          path.join(executableDir, 'libwarpdeck.dylib'),
          // Development path from Flutter project
          '../../../libwarpdeck/build/libwarpdeck.dylib',
          // Bundled with app (relative)
          'libwarpdeck.dylib',
          // Absolute development path
          '/Users/jesse/code/WarpDeck/libwarpdeck/build/libwarpdeck.dylib',
        ];
        
        DynamicLibrary? lib;
        
        for (final path in possiblePaths) {
          try {
            lib = DynamicLibrary.open(path);
            break;
          } catch (e) {
            continue;
          }
        }
        
        if (lib == null) {
          throw Exception('Could not load libwarpdeck.dylib from any of the attempted paths: ${possiblePaths.join(', ')}');
        }
        
        _lib = lib;
      } else if (Platform.isLinux) {
        // Try multiple paths for the .so file
        final executableDir = path.dirname(Platform.resolvedExecutable);
        final possiblePaths = [
          // Bundled with AppImage in same directory as executable
          path.join(executableDir, 'libwarpdeck.so'),
          // Development path from Flutter project
          '../../../libwarpdeck/build/libwarpdeck.so',
          // Bundled with app (relative)
          'libwarpdeck.so',
          // System library paths
          '/usr/local/lib/libwarpdeck.so',
          '/usr/lib/libwarpdeck.so',
        ];
        
        DynamicLibrary? lib;
        
        for (final path in possiblePaths) {
          try {
            lib = DynamicLibrary.open(path);
            break;
          } catch (e) {
            continue;
          }
        }
        
        if (lib == null) {
          throw Exception('Could not load libwarpdeck.so from any of the attempted paths: ${possiblePaths.join(', ')}');
        }
        
        _lib = lib;
      } else {
        throw UnsupportedError('Platform not supported');
      }
    } catch (e) {
      rethrow;
    }
  }

  void _bindFunctions() {
    try {
      warpdeckCreate = _lib.lookupFunction<WarpDeckCreateNative, WarpDeckCreateDart>('warpdeck_create');
      warpdeckStart = _lib.lookupFunction<WarpDeckStartNative, WarpDeckStartDart>('warpdeck_start');
      warpdeckStop = _lib.lookupFunction<WarpDeckStopNative, WarpDeckStopDart>('warpdeck_stop');
      warpdeckDestroy = _lib.lookupFunction<WarpDeckDestroyNative, WarpDeckDestroyDart>('warpdeck_destroy');
      warpdeckInitiateTransfer = _lib.lookupFunction<WarpDeckInitiateTransferNative, WarpDeckInitiateTransferDart>('warpdeck_initiate_transfer');
      warpdeckRespondToTransfer = _lib.lookupFunction<WarpDeckRespondToTransferNative, WarpDeckRespondToTransferDart>('warpdeck_respond_to_transfer');
    } catch (e) {
      rethrow;
    }
  }
}