import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:path_provider/path_provider.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:uuid/uuid.dart';

import '../models/peer.dart';
import '../models/transfer.dart';
import 'libwarpdeck_ffi.dart';

enum WarpDeckStatus {
  uninitialized,
  initialized,
  starting,
  running,
  error,
}

class WarpDeckState {
  final WarpDeckStatus status;
  final String? deviceName;
  final int? currentPort;
  final Map<String, Peer> discoveredPeers;
  final Map<String, Transfer> activeTransfers;
  final String? errorMessage;

  const WarpDeckState({
    this.status = WarpDeckStatus.uninitialized,
    this.deviceName,
    this.currentPort,
    this.discoveredPeers = const {},
    this.activeTransfers = const {},
    this.errorMessage,
  });

  WarpDeckState copyWith({
    WarpDeckStatus? status,
    String? deviceName,
    int? currentPort,
    Map<String, Peer>? discoveredPeers,
    Map<String, Transfer>? activeTransfers,
    String? errorMessage,
  }) {
    return WarpDeckState(
      status: status ?? this.status,
      deviceName: deviceName ?? this.deviceName,
      currentPort: currentPort ?? this.currentPort,
      discoveredPeers: discoveredPeers ?? this.discoveredPeers,
      activeTransfers: activeTransfers ?? this.activeTransfers,
      errorMessage: errorMessage ?? this.errorMessage,
    );
  }
}

// Riverpod provider
final warpDeckServiceProvider = StateNotifierProvider<WarpDeckService, WarpDeckState>((ref) {
  return WarpDeckService();
});

class WarpDeckService extends StateNotifier<WarpDeckState> {
  static const _uuid = Uuid();

  // Static instance reference for callbacks
  static WarpDeckService? _instance;

  Pointer<WarpDeckHandle>? _handle;
  Pointer<Callbacks>? _callbacks;
  late String _configDir;
  String _deviceName = '';
  String _ownDeviceId = '';
  bool _isStarted = false;

  // NativeCallable listeners - these can be invoked from any thread safely
  NativeCallable<on_peer_discovered_native>? _onPeerDiscoveredListener;
  NativeCallable<on_peer_lost_native>? _onPeerLostListener;
  NativeCallable<on_incoming_transfer_request_native>? _onIncomingTransferRequestListener;
  NativeCallable<on_transfer_progress_update_native>? _onTransferProgressUpdateListener;
  NativeCallable<on_transfer_completed_native>? _onTransferCompletedListener;
  NativeCallable<on_error_native>? _onErrorListener;

  WarpDeckService() : super(const WarpDeckState()) {
    _instance = this;
    _initializeConfigDir();
  }

  Future<void> _initializeConfigDir() async {
    if (Platform.isMacOS) {
      final homeDir = Platform.environment['HOME']!;
      _configDir = '$homeDir/Library/Application Support/WarpDeck';
    } else if (Platform.isLinux) {
      final homeDir = Platform.environment['HOME']!;
      _configDir = '$homeDir/.config/warpdeck';
    } else {
      final appDir = await getApplicationSupportDirectory();
      _configDir = '${appDir.path}/warpdeck';
    }
    
    // Ensure config directory exists
    await Directory(_configDir).create(recursive: true);
  }

  Future<bool> initialize({String? deviceName}) async {
    try {
      await _initializeConfigDir();
      
      _deviceName = deviceName ?? await _getStoredDeviceName() ?? _getDefaultDeviceName();
      
      // Try to initialize the FFI library first to catch loading errors
      try {
        // This will trigger library loading and function binding
        final _ = WarpDeckFFI.instance;
      } catch (e) {
        state = state.copyWith(
          status: WarpDeckStatus.error,
          errorMessage: 'Failed to load WarpDeck library: $e',
        );
        return false;
      }
      
      // Setup callbacks using NativeCallable.listener which can be called from any thread
      _onPeerDiscoveredListener = NativeCallable<on_peer_discovered_native>.listener(_onPeerDiscovered);
      _onPeerLostListener = NativeCallable<on_peer_lost_native>.listener(_onPeerLost);
      _onIncomingTransferRequestListener = NativeCallable<on_incoming_transfer_request_native>.listener(_onIncomingTransferRequest);
      _onTransferProgressUpdateListener = NativeCallable<on_transfer_progress_update_native>.listener(_onTransferProgressUpdate);
      _onTransferCompletedListener = NativeCallable<on_transfer_completed_native>.listener(_onTransferCompleted);
      _onErrorListener = NativeCallable<on_error_native>.listener(_onError);

      _callbacks = calloc<Callbacks>();
      _callbacks!.ref.on_peer_discovered = _onPeerDiscoveredListener!.nativeFunction;
      _callbacks!.ref.on_peer_lost = _onPeerLostListener!.nativeFunction;
      _callbacks!.ref.on_incoming_transfer_request = _onIncomingTransferRequestListener!.nativeFunction;
      _callbacks!.ref.on_transfer_progress_update = _onTransferProgressUpdateListener!.nativeFunction;
      _callbacks!.ref.on_transfer_completed = _onTransferCompletedListener!.nativeFunction;
      _callbacks!.ref.on_error = _onErrorListener!.nativeFunction;

      // Create WarpDeck handle
      print('Creating WarpDeck handle with config dir: $_configDir');
      final configDirPtr = _configDir.toNativeUtf8();
      _handle = WarpDeckFFI.instance.warpdeckCreate(_callbacks!, configDirPtr.cast<Char>());
      calloc.free(configDirPtr);

      if (_handle == nullptr) {
        state = state.copyWith(
          status: WarpDeckStatus.error,
          errorMessage: 'Failed to create WarpDeck instance - libwarpdeck returned null handle',
        );
        return false;
      }
      
      print('WarpDeck handle created successfully: $_handle');

      state = state.copyWith(
        status: WarpDeckStatus.initialized,
        deviceName: _deviceName,
        errorMessage: null, // Clear any previous errors
      );
      
      return true;
    } catch (e) {
      state = state.copyWith(
        status: WarpDeckStatus.error,
        errorMessage: 'Initialization failed: $e',
      );
      return false;
    }
  }

  Future<bool> start({int port = 0}) async {
    if (_handle == nullptr) {
      await initialize();
    }
    
    if (_handle == nullptr) return false;

    try {
      // Set status to starting to show progress
      state = state.copyWith(
        status: WarpDeckStatus.starting,
        errorMessage: null,
      );

      final deviceNamePtr = _deviceName.toNativeUtf8();
      
      // Wrap the FFI call with a timeout to prevent indefinite hanging
      final result = await Future.microtask(() {
        try {
          print('Calling warpdeckStart with device name: $_deviceName, port: $port');
          return WarpDeckFFI.instance.warpdeckStart(_handle!, deviceNamePtr.cast<Char>(), port);
        } catch (e) {
          print('warpdeckStart threw exception: $e');
          return -1;
        }
      }).timeout(
        const Duration(seconds: 10),
        onTimeout: () {
          return -2; // Special code for timeout
        },
      );
      
      calloc.free(deviceNamePtr);

      if (result == -2) {
        state = state.copyWith(
          status: WarpDeckStatus.error,
          errorMessage: 'WarpDeck start operation timed out (10s). Check firewall/network settings.',
        );
        return false;
      } else if (result < 0) {
        print('warpdeckStart returned error code: $result');
        state = state.copyWith(
          status: WarpDeckStatus.error,
          errorMessage: 'Failed to start WarpDeck on port $port (code: $result)',
        );
        return false;
      }
      
      print('warpdeckStart returned port: $result');

      _isStarted = true;

      // Get our own device ID for filtering self-discovery
      final status = getDiscoveryStatus();
      if (status != null && status['device_id'] != null) {
        _ownDeviceId = status['device_id'] as String;
        print('Own device ID: $_ownDeviceId');
      }

      state = state.copyWith(
        status: WarpDeckStatus.running,
        currentPort: result,
        errorMessage: null,
      );

      return true;
    } catch (e) {
      state = state.copyWith(
        status: WarpDeckStatus.error,
        errorMessage: 'Start failed: $e',
      );
      return false;
    }
  }

  Future<void> restart() async {
    if (_handle == nullptr) return;
    final port = state.currentPort ?? 0;
    await stop();
    await start(port: port);
  }

  Future<void> stop() async {
    if (_handle != nullptr && _isStarted) {
      WarpDeckFFI.instance.warpdeckStop(_handle!);
      _isStarted = false;
      
      state = state.copyWith(
        status: WarpDeckStatus.initialized,
        currentPort: null,
        discoveredPeers: {},
      );
    }
  }

  @override
  Future<void> dispose() async {
    await stop();

    if (_handle != nullptr) {
      WarpDeckFFI.instance.warpdeckDestroy(_handle!);
      _handle = nullptr;
    }

    super.dispose();

    if (_callbacks != nullptr) {
      calloc.free(_callbacks!);
      _callbacks = nullptr;
    }

    // Close NativeCallable listeners
    _onPeerDiscoveredListener?.close();
    _onPeerLostListener?.close();
    _onIncomingTransferRequestListener?.close();
    _onTransferProgressUpdateListener?.close();
    _onTransferCompletedListener?.close();
    _onErrorListener?.close();

    state = state.copyWith(status: WarpDeckStatus.uninitialized);
  }

  Future<void> sendFiles(String targetPeerId, List<String> filePaths) async {
    if (_handle == nullptr || !_isStarted) return;

    try {
      // Create file list JSON
      final files = <Map<String, dynamic>>[];
      for (final filePath in filePaths) {
        final file = File(filePath);
        if (await file.exists()) {
          final stat = await file.stat();
          files.add({
            'name': file.uri.pathSegments.last,
            'size': stat.size,
            'path': filePath,
          });
        }
      }

      final filesJson = jsonEncode(files);
      final targetIdPtr = targetPeerId.toNativeUtf8();
      final filesJsonPtr = filesJson.toNativeUtf8();

      WarpDeckFFI.instance.warpdeckInitiateTransfer(_handle!, targetIdPtr.cast<Char>(), filesJsonPtr.cast<Char>());

      calloc.free(targetIdPtr);
      calloc.free(filesJsonPtr);

      // Create transfer record
      final transfer = Transfer(
        id: _uuid.v4(),
        peerId: targetPeerId,
        peerName: state.discoveredPeers[targetPeerId]?.name ?? 'Unknown',
        files: files.map((f) => FileInfo.fromJson(f)).toList(),
        direction: TransferDirection.outgoing,
        status: TransferStatus.pending,
        totalBytes: files.fold(0, (sum, f) => sum + (f['size'] as int)),
        createdAt: DateTime.now(),
      );

      state = state.copyWith(
        activeTransfers: {...state.activeTransfers, transfer.id: transfer},
      );
    } catch (e) {
      state = state.copyWith(
        status: WarpDeckStatus.error,
        errorMessage: 'Send files failed: $e',
      );
    }
  }

  Future<void> respondToTransfer(String transferId, bool accepted) async {
    if (_handle == nullptr || !_isStarted) return;

    try {
      final transferIdPtr = transferId.toNativeUtf8();
      WarpDeckFFI.instance.warpdeckRespondToTransfer(_handle!, transferIdPtr.cast<Char>(), accepted);
      calloc.free(transferIdPtr);

      // Update transfer status
      final transfer = state.activeTransfers[transferId];
      if (transfer != null) {
        final updatedTransfer = transfer.copyWith(
          status: accepted ? TransferStatus.inProgress : TransferStatus.cancelled,
        );
        state = state.copyWith(
          activeTransfers: {...state.activeTransfers, transferId: updatedTransfer},
        );
      }
    } catch (e) {
      state = state.copyWith(
        status: WarpDeckStatus.error,
        errorMessage: 'Respond to transfer failed: $e',
      );
    }
  }

  Future<void> setDeviceName(String name) async {
    _deviceName = name;
    await _saveDeviceName(name);
    
    state = state.copyWith(deviceName: name);
    
    // Restart if running to update broadcast name
    if (_isStarted) {
      await stop();
      await start();
    }
  }

  // Static callback functions - these use _instance to update state
  // NOTE: String pointers from C++ are heap-allocated with strdup() and must be freed
  static void _onPeerDiscovered(Pointer<Char> peerJsonPtr) {
    try {
      final peerJson = peerJsonPtr.cast<Utf8>().toDartString();
      print('_onPeerDiscovered: $peerJson');

      // Free the heap-allocated string from C++
      WarpDeckFFI.instance.warpdeckFreeString(peerJsonPtr);

      final instance = _instance;
      if (instance == null) {
        print('Warning: WarpDeckService instance is null in _onPeerDiscovered');
        return;
      }

      final peerData = jsonDecode(peerJson) as Map<String, dynamic>;
      final peer = Peer.fromJson(peerData);

      // Filter out self-discovery - don't show ourselves in the peer list
      if (peer.id == instance._ownDeviceId) {
        print('Filtered out self-discovery: ${peer.id}');
        return;
      }

      // Update state with new peer
      final currentPeers = Map<String, Peer>.from(instance.state.discoveredPeers);
      currentPeers[peer.id] = peer;
      instance.state = instance.state.copyWith(discoveredPeers: currentPeers);

      print('Peer added to state: ${peer.name} (${peer.id})');
    } catch (e) {
      print('Error in _onPeerDiscovered: $e');
    }
  }

  static void _onPeerLost(Pointer<Char> deviceIdPtr) {
    try {
      final deviceId = deviceIdPtr.cast<Utf8>().toDartString();
      print('_onPeerLost: $deviceId');

      // Free the heap-allocated string from C++
      WarpDeckFFI.instance.warpdeckFreeString(deviceIdPtr);

      final instance = _instance;
      if (instance == null) {
        print('Warning: WarpDeckService instance is null in _onPeerLost');
        return;
      }

      // Remove peer from state
      final currentPeers = Map<String, Peer>.from(instance.state.discoveredPeers);
      currentPeers.remove(deviceId);
      instance.state = instance.state.copyWith(discoveredPeers: currentPeers);

      print('Peer removed from state: $deviceId');
    } catch (e) {
      print('Error in _onPeerLost: $e');
    }
  }

  static void _onIncomingTransferRequest(Pointer<Char> transferRequestJsonPtr) {
    try {
      final requestJson = transferRequestJsonPtr.cast<Utf8>().toDartString();
      print('_onIncomingTransferRequest: $requestJson');

      // Free the heap-allocated string from C++
      WarpDeckFFI.instance.warpdeckFreeString(transferRequestJsonPtr);

      final instance = _instance;
      if (instance == null) return;

      final requestData = jsonDecode(requestJson) as Map<String, dynamic>;
      final files = (requestData['files'] as List?)
          ?.map((f) => FileInfo.fromJson(f as Map<String, dynamic>))
          .toList() ?? [];

      final transfer = Transfer(
        id: requestData['transfer_id'] ?? _uuid.v4(),
        peerId: requestData['peer_id'] ?? 'unknown',
        peerName: requestData['peer_name'] ?? 'Unknown',
        files: files,
        direction: TransferDirection.incoming,
        status: TransferStatus.pending,
        totalBytes: files.fold(0, (sum, f) => sum + f.size),
        createdAt: DateTime.now(),
      );

      final currentTransfers = Map<String, Transfer>.from(instance.state.activeTransfers);
      currentTransfers[transfer.id] = transfer;
      instance.state = instance.state.copyWith(activeTransfers: currentTransfers);
    } catch (e) {
      print('Error in _onIncomingTransferRequest: $e');
    }
  }

  static void _onTransferProgressUpdate(Pointer<Char> transferIdPtr, double progress, int bytesTransferred) {
    try {
      final transferId = transferIdPtr.cast<Utf8>().toDartString();
      print('_onTransferProgressUpdate: $transferId - $progress%');

      // Free the heap-allocated string from C++
      WarpDeckFFI.instance.warpdeckFreeString(transferIdPtr);

      final instance = _instance;
      if (instance == null) return;

      final transfer = instance.state.activeTransfers[transferId];
      if (transfer != null) {
        final updatedTransfer = transfer.copyWith(
          status: TransferStatus.inProgress,
          bytesTransferred: bytesTransferred,
        );
        final currentTransfers = Map<String, Transfer>.from(instance.state.activeTransfers);
        currentTransfers[transferId] = updatedTransfer;
        instance.state = instance.state.copyWith(activeTransfers: currentTransfers);
      }
    } catch (e) {
      print('Error in _onTransferProgressUpdate: $e');
    }
  }

  static void _onTransferCompleted(Pointer<Char> transferIdPtr, bool success, Pointer<Char> errorMessagePtr) {
    try {
      final transferId = transferIdPtr.cast<Utf8>().toDartString();
      final errorMessage = errorMessagePtr != nullptr
          ? errorMessagePtr.cast<Utf8>().toDartString()
          : null;
      print('_onTransferCompleted: $transferId - success: $success, error: $errorMessage');

      // Free the heap-allocated strings from C++
      WarpDeckFFI.instance.warpdeckFreeString(transferIdPtr);
      if (errorMessagePtr != nullptr) {
        WarpDeckFFI.instance.warpdeckFreeString(errorMessagePtr);
      }

      final instance = _instance;
      if (instance == null) return;

      final transfer = instance.state.activeTransfers[transferId];
      if (transfer != null) {
        final updatedTransfer = transfer.copyWith(
          status: success ? TransferStatus.completed : TransferStatus.failed,
          completedAt: DateTime.now(),
        );
        final currentTransfers = Map<String, Transfer>.from(instance.state.activeTransfers);
        currentTransfers[transferId] = updatedTransfer;
        instance.state = instance.state.copyWith(activeTransfers: currentTransfers);
      }
    } catch (e) {
      print('Error in _onTransferCompleted: $e');
    }
  }

  static void _onError(Pointer<Char> errorMessagePtr) {
    try {
      final errorMessage = errorMessagePtr.cast<Utf8>().toDartString();
      print('_onError: $errorMessage');

      final instance = _instance;
      if (instance == null) return;

      instance.state = instance.state.copyWith(errorMessage: errorMessage);
    } catch (e) {
      print('Error in _onError: $e');
    }
  }

  // Helper methods
  String _getDefaultDeviceName() {
    if (Platform.isMacOS) return 'Mac Flutter';
    if (Platform.isLinux) return 'Linux Flutter';
    return 'WarpDeck Flutter';
  }

  Future<String?> _getStoredDeviceName() async {
    try {
      final configFile = File('$_configDir/config.json');
      if (await configFile.exists()) {
        final content = await configFile.readAsString();
        final config = jsonDecode(content) as Map<String, dynamic>;
        return config['device_name'] as String?;
      }
    } catch (e) {
      // Ignore errors reading config
    }
    return null;
  }

  Future<void> _saveDeviceName(String name) async {
    try {
      final configFile = File('$_configDir/config.json');
      Map<String, dynamic> config = {};
      
      if (await configFile.exists()) {
        final content = await configFile.readAsString();
        config = jsonDecode(content) as Map<String, dynamic>;
      }
      
      config['device_name'] = name;
      await configFile.writeAsString(jsonEncode(config));
    } catch (e) {
      // Ignore errors saving config
    }
  }

  // Debug methods for MdnsManager integration
  Map<String, dynamic>? getDiscoveryStatus() {
    if (_handle == nullptr) return null;
    
    try {
      final statusJsonPtr = WarpDeckFFI.instance.warpdeckGetDiscoveryStatus(_handle!);
      if (statusJsonPtr == nullptr) return null;
      final statusJson = statusJsonPtr.cast<Utf8>().toDartString();
      WarpDeckFFI.instance.warpdeckFreeString(statusJsonPtr);
      return jsonDecode(statusJson) as Map<String, dynamic>;
    } catch (e) {
      return null;
    }
  }

  Map<String, dynamic>? getDiscoveredPeersDebugInfo() {
    if (_handle == nullptr) return null;
    
    try {
      final peersJsonPtr = WarpDeckFFI.instance.warpdeckGetDiscoveredPeers(_handle!);
      if (peersJsonPtr == nullptr) return null;
      final peersJson = peersJsonPtr.cast<Utf8>().toDartString();
      WarpDeckFFI.instance.warpdeckFreeString(peersJsonPtr);
      return jsonDecode(peersJson) as Map<String, dynamic>;
    } catch (e) {
      return null;
    }
  }

  String? getMdnsDebugInfo() {
    if (_handle == nullptr) return null;

    try {
      final debugInfoPtr = WarpDeckFFI.instance.warpdeckGetMdnsDebugInfo(_handle!);
      if (debugInfoPtr == nullptr) return null;
      final debugInfo = debugInfoPtr.cast<Utf8>().toDartString();
      WarpDeckFFI.instance.warpdeckFreeString(debugInfoPtr);
      return debugInfo;
    } catch (e) {
      return null;
    }
  }

  // ============================================================================
  // Transfer Queue Methods
  // ============================================================================

  /// Queue files for transfer to a peer. Returns the queue ID on success, null on failure.
  /// Files should be a list of maps with 'path' and optionally 'relative_path' keys.
  Future<String?> queueFiles(String targetPeerId, List<Map<String, dynamic>> files) async {
    if (_handle == nullptr || !_isStarted) return null;

    try {
      final filesJson = jsonEncode(files);
      final targetIdPtr = targetPeerId.toNativeUtf8();
      final filesJsonPtr = filesJson.toNativeUtf8();

      final queueIdPtr = WarpDeckFFI.instance.warpdeckQueueTransfer(
        _handle!,
        targetIdPtr.cast<Char>(),
        filesJsonPtr.cast<Char>(),
      );

      calloc.free(targetIdPtr);
      calloc.free(filesJsonPtr);

      if (queueIdPtr == nullptr) return null;

      final queueId = queueIdPtr.cast<Utf8>().toDartString();
      WarpDeckFFI.instance.warpdeckFreeString(queueIdPtr);

      return queueId;
    } catch (e) {
      print('queueFiles failed: $e');
      return null;
    }
  }

  /// Cancel a queued or active transfer by queue ID.
  bool cancelQueuedTransfer(String queueId) {
    if (_handle == nullptr) return false;

    try {
      final queueIdPtr = queueId.toNativeUtf8();
      final result = WarpDeckFFI.instance.warpdeckCancelQueuedTransfer(
        _handle!,
        queueIdPtr.cast<Char>(),
      );
      calloc.free(queueIdPtr);
      return result;
    } catch (e) {
      print('cancelQueuedTransfer failed: $e');
      return false;
    }
  }

  /// Get the current queue status as a list of QueuedTransfer objects.
  List<QueuedTransfer> getQueueStatus() {
    if (_handle == nullptr) return [];

    try {
      final statusJsonPtr = WarpDeckFFI.instance.warpdeckGetQueueStatus(_handle!);
      if (statusJsonPtr == nullptr) return [];

      final statusJson = statusJsonPtr.cast<Utf8>().toDartString();
      WarpDeckFFI.instance.warpdeckFreeString(statusJsonPtr);

      final List<dynamic> statusList = jsonDecode(statusJson);
      return statusList
          .map((item) => QueuedTransfer.fromJson(item as Map<String, dynamic>))
          .toList();
    } catch (e) {
      print('getQueueStatus failed: $e');
      return [];
    }
  }

  // ============================================================================
  // Folder Transfer Helpers
  // ============================================================================

  /// Recursively gather all files from a directory, preserving relative paths.
  /// Returns a list of maps with 'path', 'relative_path', 'name', and 'size' keys.
  Future<List<Map<String, dynamic>>> gatherFilesFromDirectory(String rootDirPath) async {
    final rootDir = Directory(rootDirPath);
    if (!await rootDir.exists()) return [];

    final rootName = rootDir.uri.pathSegments.where((s) => s.isNotEmpty).last;
    final files = <Map<String, dynamic>>[];

    await for (final entity in rootDir.list(recursive: true, followLinks: false)) {
      if (entity is File) {
        try {
          final stat = await entity.stat();
          final absolutePath = entity.path;

          // Calculate relative path from root directory
          // e.g., if rootDirPath is /home/user/MyFolder and file is /home/user/MyFolder/sub/file.txt
          // then relativePath should be MyFolder/sub/file.txt
          final relativeFromRoot = absolutePath.substring(rootDirPath.length);
          final relativePath = '$rootName${relativeFromRoot.startsWith('/') ? relativeFromRoot : '/$relativeFromRoot'}';

          files.add({
            'path': absolutePath,
            'relative_path': relativePath,
            'name': entity.uri.pathSegments.last,
            'size': stat.size,
          });
        } catch (e) {
          // Skip files we can't access (permission denied, etc.)
          print('Skipping file ${entity.path}: $e');
        }
      }
    }

    return files;
  }

  /// Send a folder to a peer, preserving directory structure.
  Future<String?> sendFolder(String targetPeerId, String folderPath) async {
    final files = await gatherFilesFromDirectory(folderPath);
    if (files.isEmpty) {
      print('No files found in folder: $folderPath');
      return null;
    }

    return queueFiles(targetPeerId, files);
  }
}