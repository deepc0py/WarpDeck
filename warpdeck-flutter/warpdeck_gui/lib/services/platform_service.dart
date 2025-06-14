import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import '../utils/app_theme.dart';

class PlatformInfo {
  final bool isSteamDeck;
  final bool isDesktop;
  final bool isHandheld;
  final String deviceType;
  final double screenDiagonal;
  final bool hasGamepadSupport;

  const PlatformInfo({
    required this.isSteamDeck,
    required this.isDesktop,
    required this.isHandheld,
    required this.deviceType,
    required this.screenDiagonal,
    required this.hasGamepadSupport,
  });
}

class PlatformService {
  static const PlatformService _instance = PlatformService._internal();
  factory PlatformService() => _instance;
  const PlatformService._internal();

  static PlatformInfo? _cachedInfo;

  /// Get platform information with Steam Deck detection
  PlatformInfo getPlatformInfo() {
    if (_cachedInfo != null) return _cachedInfo!;

    final isSteamDeck = _detectSteamDeck();
    final isDesktop = Platform.isLinux || Platform.isMacOS || Platform.isWindows;
    
    _cachedInfo = PlatformInfo(
      isSteamDeck: isSteamDeck,
      isDesktop: isDesktop,
      isHandheld: isSteamDeck,
      deviceType: _getDeviceType(isSteamDeck),
      screenDiagonal: isSteamDeck ? 7.0 : 15.0, // Steam Deck is 7", assume 15" for desktop
      hasGamepadSupport: isSteamDeck || Platform.isLinux || Platform.isWindows,
    );

    return _cachedInfo!;
  }

  /// Detect if running on Steam Deck with robust error handling
  bool _detectSteamDeck() {
    if (!Platform.isLinux) return false;

    var steamDeckScore = 0;
    var detectionMethods = <String>[];

    try {
      // Method 1: DMI detection (most reliable)
      const dmiPaths = [
        '/sys/devices/virtual/dmi/id/product_name',
        '/sys/devices/virtual/dmi/id/board_name',
      ];

      for (final path in dmiPaths) {
        try {
          final file = File(path);
          if (file.existsSync()) {
            final content = file.readAsStringSync().toLowerCase().trim();
            if (content.contains('jupiter') || content.contains('steamdeck')) {
              steamDeckScore += 10;
              detectionMethods.add('DMI: $path');
            }
          }
        } catch (e) {
          if (kDebugMode) print('DMI check failed for $path: $e');
        }
      }

      // Method 2: Steam directory (less reliable, but good indicator)
      try {
        if (Directory('/home/deck/.steam').existsSync()) {
          steamDeckScore += 5;
          detectionMethods.add('Steam directory');
        }
      } catch (e) {
        if (kDebugMode) print('Steam directory check failed: $e');
      }

      // Method 3: Environment variables
      const steamDeckEnvs = ['STEAM_COMPAT_DATA_PATH', 'SteamDeck', 'GAMESCOPE_WAYLAND_DISPLAY'];
      for (final envVar in steamDeckEnvs) {
        if (Platform.environment.containsKey(envVar)) {
          steamDeckScore += 3;
          detectionMethods.add('ENV: $envVar');
        }
      }

      // Method 4: Process detection (least reliable)
      try {
        final result = Process.runSync('pgrep', ['-f', 'gamescope|steamos']);
        if (result.exitCode == 0) {
          steamDeckScore += 2;
          detectionMethods.add('Process detection');
        }
      } catch (e) {
        if (kDebugMode) print('Process detection failed: $e');
      }

      // Method 5: Hardware detection (CPU model)
      try {
        final cpuInfo = File('/proc/cpuinfo');
        if (cpuInfo.existsSync()) {
          final content = cpuInfo.readAsStringSync().toLowerCase();
          if (content.contains('amd custom apu') || content.contains('van gogh')) {
            steamDeckScore += 7;
            detectionMethods.add('CPU detection');
          }
        }
      } catch (e) {
        if (kDebugMode) print('CPU detection failed: $e');
      }

      final isSteamDeck = steamDeckScore >= 10;
      
      if (kDebugMode) {
        print('Steam Deck detection score: $steamDeckScore (threshold: 10)');
        print('Detection methods: ${detectionMethods.join(', ')}');
        print('Result: ${isSteamDeck ? 'Steam Deck detected' : 'Not Steam Deck'}');
      }

      return isSteamDeck;

    } catch (e) {
      if (kDebugMode) print('Steam Deck detection error: $e');
      return false;
    }
  }

  String _getDeviceType(bool isSteamDeck) {
    if (isSteamDeck) return 'Steam Deck';
    if (Platform.isMacOS) return 'macOS';
    if (Platform.isLinux) return 'Linux Desktop';
    if (Platform.isWindows) return 'Windows';
    return 'Unknown';
  }

  /// Get optimal UI scale for the platform
  double getUIScale() {
    final info = getPlatformInfo();
    
    if (info.isSteamDeck) {
      return 1.25; // Slightly larger for handheld use
    }
    
    return 1.0; // Normal scale for desktop
  }

  /// Get optimal touch target size
  double getTouchTargetSize() {
    final info = getPlatformInfo();
    
    if (info.isSteamDeck) {
      return 48.0; // Larger touch targets for handheld
    }
    
    return 40.0; // Standard desktop size
  }

  /// Check if gamepad navigation should be enabled
  bool shouldEnableGamepadNavigation() {
    return getPlatformInfo().hasGamepadSupport;
  }

  /// Get platform-specific optimizations
  Map<String, dynamic> getOptimizations() {
    final info = getPlatformInfo();
    
    return {
      'enableGamepadNavigation': info.hasGamepadSupport,
      'largerTouchTargets': info.isHandheld,
      'batteryOptimizations': info.isHandheld,
      'gamingModeUI': info.isSteamDeck,
      'autoHideControls': info.isSteamDeck,
      'hapticFeedback': info.isHandheld,
    };
  }

  /// Steam Deck specific: Check if in Gaming Mode
  bool isInGamingMode() {
    if (!getPlatformInfo().isSteamDeck) return false;
    
    try {
      // Check if Steam is running in Big Picture mode
      final result = Process.runSync('pgrep', ['-f', 'steamwebhelper']);
      return result.exitCode == 0;
    } catch (e) {
      return false;
    }
  }

  /// Steam Deck specific: Get battery level
  int? getBatteryLevel() {
    if (!getPlatformInfo().isSteamDeck) return null;
    
    try {
      final batteryPath = '/sys/class/power_supply/jupiter-main-battery/capacity';
      final file = File(batteryPath);
      if (file.existsSync()) {
        return int.tryParse(file.readAsStringSync().trim());
      }
    } catch (e) {
      if (kDebugMode) {
        print('Error reading battery level: $e');
      }
    }
    
    return null;
  }

  /// Steam Deck specific: Check if plugged in
  bool? isPluggedIn() {
    if (!getPlatformInfo().isSteamDeck) return null;
    
    try {
      final acPath = '/sys/class/power_supply/jupiter-pd-adapter/online';
      final file = File(acPath);
      if (file.existsSync()) {
        return file.readAsStringSync().trim() == '1';
      }
    } catch (e) {
      if (kDebugMode) {
        print('Error checking AC adapter: $e');
      }
    }
    
    return null;
  }

  /// Get optimal theme based on platform and screen size
  ThemeData getOptimalTheme(Size screenSize, {bool isDarkMode = false}) {
    final info = getPlatformInfo();
    
    // Base theme
    ThemeData baseTheme = isDarkMode ? AppTheme.darkTheme : AppTheme.lightTheme;
    
    // Apply Steam Deck optimizations if detected
    if (info.isSteamDeck) {
      return SteamDeckTheme.getSteamDeckTheme(baseTheme);
    }
    
    // Apply responsive optimizations based on screen size
    if (ResponsiveBreakpoints.isSteamDeckResolution(screenSize)) {
      // Even if not detected as Steam Deck, optimize for the resolution
      return SteamDeckTheme.getSteamDeckTheme(baseTheme);
    }
    
    return baseTheme;
  }

  /// Get layout configuration based on screen size and platform
  Map<String, dynamic> getLayoutConfig(Size screenSize) {
    final info = getPlatformInfo();
    
    return {
      'columnCount': ResponsiveBreakpoints.getColumnCount(screenSize),
      'padding': ResponsiveBreakpoints.getScreenPadding(screenSize),
      'spacing': ResponsiveBreakpoints.getSpacing(screenSize),
      'isSteamDeckOptimized': info.isSteamDeck || ResponsiveBreakpoints.isSteamDeckResolution(screenSize),
      'touchTargetSize': getTouchTargetSize(),
      'uiScale': getUIScale(),
      'isHandheldMode': info.isHandheld || ResponsiveBreakpoints.isSteamDeckResolution(screenSize),
    };
  }

  /// Get Steam Deck specific gaming mode theme
  ThemeData? getSteamDeckGamingTheme() {
    if (!getPlatformInfo().isSteamDeck) return null;
    return SteamDeckTheme.steamDeckGamingTheme;
  }
}