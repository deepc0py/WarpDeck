import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:http/http.dart' as http;
import 'package:package_info_plus/package_info_plus.dart';
import 'package:version/version.dart';

class UpdateInfo {
  final String version;
  final String releaseNotes;
  final String downloadUrl;
  final bool isCritical;
  final DateTime releaseDate;

  UpdateInfo({
    required this.version,
    required this.releaseNotes,
    required this.downloadUrl,
    this.isCritical = false,
    required this.releaseDate,
  });

  factory UpdateInfo.fromJson(Map<String, dynamic> json) {
    return UpdateInfo(
      version: json['tag_name'] ?? '',
      releaseNotes: json['body'] ?? '',
      downloadUrl: _getDownloadUrl(json['assets'] ?? []),
      isCritical: (json['body'] ?? '').toLowerCase().contains('critical'),
      releaseDate: DateTime.parse(json['published_at'] ?? DateTime.now().toIso8601String()),
    );
  }

  static String _getDownloadUrl(List<dynamic> assets) {
    final String platform = Platform.isMacOS ? 'macOS' : 'linux';
    final String extension = Platform.isMacOS ? '.dmg' : '.AppImage';
    
    for (final asset in assets) {
      final String name = asset['name'] ?? '';
      if (name.toLowerCase().contains(platform.toLowerCase()) && 
          name.toLowerCase().contains(extension)) {
        return asset['browser_download_url'] ?? '';
      }
    }
    return '';
  }
}

enum UpdateStatus {
  checking,
  available,
  noUpdate,
  downloading,
  readyToInstall,
  error,
}

class UpdateState {
  final UpdateStatus status;
  final UpdateInfo? updateInfo;
  final String? errorMessage;
  final double downloadProgress;

  const UpdateState({
    required this.status,
    this.updateInfo,
    this.errorMessage,
    this.downloadProgress = 0.0,
  });

  UpdateState copyWith({
    UpdateStatus? status,
    UpdateInfo? updateInfo,
    String? errorMessage,
    double? downloadProgress,
  }) {
    return UpdateState(
      status: status ?? this.status,
      updateInfo: updateInfo ?? this.updateInfo,
      errorMessage: errorMessage ?? this.errorMessage,
      downloadProgress: downloadProgress ?? this.downloadProgress,
    );
  }
}

class UpdateService extends StateNotifier<UpdateState> {
  static const String _githubApiUrl = 'https://api.github.com/repos/deepc0py/WarpDeck/releases/latest';
  static const Duration _checkInterval = Duration(hours: 6);
  
  UpdateService() : super(const UpdateState(status: UpdateStatus.noUpdate));

  /// Check for updates manually
  Future<void> checkForUpdates() async {
    if (state.status == UpdateStatus.checking) return;
    
    state = state.copyWith(status: UpdateStatus.checking);
    
    try {
      final response = await http.get(Uri.parse(_githubApiUrl));
      
      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        final updateInfo = UpdateInfo.fromJson(data);
        
        final packageInfo = await PackageInfo.fromPlatform();
        final currentVersion = Version.parse(packageInfo.version);
        final latestVersion = Version.parse(updateInfo.version.replaceFirst('v', ''));
        
        if (latestVersion > currentVersion) {
          state = state.copyWith(
            status: UpdateStatus.available,
            updateInfo: updateInfo,
          );
        } else {
          state = state.copyWith(status: UpdateStatus.noUpdate);
        }
      } else {
        state = state.copyWith(
          status: UpdateStatus.error,
          errorMessage: 'Failed to check for updates: ${response.statusCode}',
        );
      }
    } catch (e) {
      state = state.copyWith(
        status: UpdateStatus.error,
        errorMessage: 'Error checking for updates: $e',
      );
    }
  }

  /// Download and prepare update
  Future<void> downloadUpdate() async {
    if (state.updateInfo == null || state.status != UpdateStatus.available) return;
    
    state = state.copyWith(status: UpdateStatus.downloading, downloadProgress: 0.0);
    
    try {
      final updateInfo = state.updateInfo!;
      
      if (updateInfo.downloadUrl.isEmpty) {
        state = state.copyWith(
          status: UpdateStatus.error,
          errorMessage: 'No download URL available for this platform',
        );
        return;
      }

      // In a real implementation, you would:
      // 1. Download the update file with progress tracking
      // 2. Verify the download integrity (checksum)
      // 3. Prepare for installation
      
      // For now, simulate download progress
      for (int i = 0; i <= 100; i += 10) {
        await Future.delayed(const Duration(milliseconds: 100));
        state = state.copyWith(downloadProgress: i / 100.0);
      }
      
      state = state.copyWith(status: UpdateStatus.readyToInstall);
      
    } catch (e) {
      state = state.copyWith(
        status: UpdateStatus.error,
        errorMessage: 'Failed to download update: $e',
      );
    }
  }

  /// Install the downloaded update
  Future<void> installUpdate() async {
    if (state.status != UpdateStatus.readyToInstall) return;
    
    // In a real implementation, you would:
    // 1. Close the current application
    // 2. Launch the installer/updater
    // 3. The installer would replace the app and restart it
    
    // For now, just open the download URL
    final updateInfo = state.updateInfo;
    if (updateInfo != null && updateInfo.downloadUrl.isNotEmpty) {
      // This would typically launch the system's default browser
      // or the platform-specific installer
      if (kDebugMode) {
        print('Would install update from: ${updateInfo.downloadUrl}');
      }
    }
  }

  /// Start automatic update checking
  void startPeriodicChecks() {
    // Check immediately
    checkForUpdates();
    
    // Then check periodically
    Stream.periodic(_checkInterval).listen((_) {
      if (state.status != UpdateStatus.checking) {
        checkForUpdates();
      }
    });
  }

  /// Reset update state
  void reset() {
    state = const UpdateState(status: UpdateStatus.noUpdate);
  }
}

final updateServiceProvider = StateNotifierProvider<UpdateService, UpdateState>((ref) {
  return UpdateService();
});

/// Provider for convenient access to update checking
final updateCheckerProvider = Provider<UpdateService>((ref) {
  return ref.read(updateServiceProvider.notifier);
});

/// Provider to check if updates should be checked automatically
final shouldCheckUpdatesProvider = Provider<bool>((ref) {
  // In a real app, this might come from user preferences
  return true;
});