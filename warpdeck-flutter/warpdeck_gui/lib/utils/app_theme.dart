import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';

class AppTheme {
  // Color palette
  static const Color primaryBlue = Color(0xFF2E86AB);
  static const Color primaryPurple = Color(0xFF6A4C93);
  static const Color accentOrange = Color(0xFFF18F01);
  static const Color accentGreen = Color(0xFF48A14D);
  static const Color surfaceLight = Color(0xFFF8F9FA);
  static const Color surfaceDark = Color(0xFF1E1E1E);

  static ThemeData get lightTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.light,
      colorScheme: ColorScheme.fromSeed(
        seedColor: primaryBlue,
        brightness: Brightness.light,
      ).copyWith(
        primary: primaryBlue,
        secondary: primaryPurple,
        tertiary: accentOrange,
        surface: surfaceLight,
        onSurface: Colors.black87,
      ),
      textTheme: GoogleFonts.interTextTheme(
        ThemeData.light().textTheme,
      ).copyWith(
        headlineLarge: GoogleFonts.inter(
          fontSize: 32,
          fontWeight: FontWeight.bold,
          color: Colors.black87,
        ),
        headlineMedium: GoogleFonts.inter(
          fontSize: 24,
          fontWeight: FontWeight.w600,
          color: Colors.black87,
        ),
        titleLarge: GoogleFonts.inter(
          fontSize: 20,
          fontWeight: FontWeight.w600,
          color: Colors.black87,
        ),
        titleMedium: GoogleFonts.inter(
          fontSize: 16,
          fontWeight: FontWeight.w500,
          color: Colors.black87,
        ),
        bodyLarge: GoogleFonts.inter(
          fontSize: 16,
          fontWeight: FontWeight.normal,
          color: Colors.black87,
        ),
        bodyMedium: GoogleFonts.inter(
          fontSize: 14,
          fontWeight: FontWeight.normal,
          color: Colors.black87,
        ),
      ),
      appBarTheme: AppBarTheme(
        elevation: 0,
        backgroundColor: surfaceLight,
        foregroundColor: Colors.black87,
        titleTextStyle: GoogleFonts.inter(
          fontSize: 20,
          fontWeight: FontWeight.w600,
          color: Colors.black87,
        ),
      ),
      cardTheme: CardTheme(
        elevation: 2,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(12),
        ),
        color: Colors.white,
      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: primaryBlue,
          foregroundColor: Colors.white,
          elevation: 2,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
          textStyle: GoogleFonts.inter(
            fontSize: 14,
            fontWeight: FontWeight.w500,
          ),
        ),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: FilledButton.styleFrom(
          backgroundColor: primaryPurple,
          foregroundColor: Colors.white,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
        ),
      ),
      outlinedButtonTheme: OutlinedButtonThemeData(
        style: OutlinedButton.styleFrom(
          foregroundColor: primaryBlue,
          side: const BorderSide(color: primaryBlue),
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
        ),
      ),
      inputDecorationTheme: InputDecorationTheme(
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(8),
          borderSide: const BorderSide(color: Colors.grey),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(8),
          borderSide: const BorderSide(color: primaryBlue, width: 2),
        ),
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      ),
    );
  }

  static ThemeData get darkTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.dark,
      colorScheme: ColorScheme.fromSeed(
        seedColor: primaryBlue,
        brightness: Brightness.dark,
      ).copyWith(
        primary: primaryBlue,
        secondary: primaryPurple,
        tertiary: accentOrange,
        surface: surfaceDark,
        onSurface: Colors.white,
      ),
      textTheme: GoogleFonts.interTextTheme(
        ThemeData.dark().textTheme,
      ).copyWith(
        headlineLarge: GoogleFonts.inter(
          fontSize: 32,
          fontWeight: FontWeight.bold,
          color: Colors.white,
        ),
        headlineMedium: GoogleFonts.inter(
          fontSize: 24,
          fontWeight: FontWeight.w600,
          color: Colors.white,
        ),
        titleLarge: GoogleFonts.inter(
          fontSize: 20,
          fontWeight: FontWeight.w600,
          color: Colors.white,
        ),
        titleMedium: GoogleFonts.inter(
          fontSize: 16,
          fontWeight: FontWeight.w500,
          color: Colors.white,
        ),
        bodyLarge: GoogleFonts.inter(
          fontSize: 16,
          fontWeight: FontWeight.normal,
          color: Colors.white,
        ),
        bodyMedium: GoogleFonts.inter(
          fontSize: 14,
          fontWeight: FontWeight.normal,
          color: Colors.white70,
        ),
      ),
      appBarTheme: AppBarTheme(
        elevation: 0,
        backgroundColor: surfaceDark,
        foregroundColor: Colors.white,
        titleTextStyle: GoogleFonts.inter(
          fontSize: 20,
          fontWeight: FontWeight.w600,
          color: Colors.white,
        ),
      ),
      cardTheme: CardTheme(
        elevation: 4,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(12),
        ),
        color: const Color(0xFF2D2D2D),
      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: primaryBlue,
          foregroundColor: Colors.white,
          elevation: 2,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
        ),
      ),
      filledButtonTheme: FilledButtonThemeData(
        style: FilledButton.styleFrom(
          backgroundColor: primaryPurple,
          foregroundColor: Colors.white,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
        ),
      ),
      outlinedButtonTheme: OutlinedButtonThemeData(
        style: OutlinedButton.styleFrom(
          foregroundColor: primaryBlue,
          side: const BorderSide(color: primaryBlue),
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
          ),
        ),
      ),
      inputDecorationTheme: InputDecorationTheme(
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(8),
          borderSide: const BorderSide(color: Colors.grey),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(8),
          borderSide: const BorderSide(color: primaryBlue, width: 2),
        ),
        contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      ),
    );
  }

  // Custom color extensions
  static const Color successGreen = Color(0xFF48A14D);
  static const Color warningYellow = Color(0xFFF59E0B);
  static const Color errorRed = Color(0xFFEF4444);
  static const Color infoBlue = Color(0xFF3B82F6);
}

/// Responsive breakpoints for different devices and form factors
class ResponsiveBreakpoints {
  // Steam Deck specific dimensions
  static const double steamDeckWidth = 1280.0;
  static const double steamDeckHeight = 800.0;
  
  // Standard responsive breakpoints
  static const double mobileBreakpoint = 600.0;
  static const double tabletBreakpoint = 900.0;
  static const double desktopBreakpoint = 1200.0;
  
  /// Check if current size matches Steam Deck resolution
  static bool isSteamDeckResolution(Size size) {
    return (size.width <= steamDeckWidth && size.height <= steamDeckHeight) ||
           (size.width <= steamDeckHeight && size.height <= steamDeckWidth);
  }
  
  /// Check if the device is in mobile form factor
  static bool isMobile(Size size) {
    return size.width < mobileBreakpoint;
  }
  
  /// Check if the device is in tablet form factor
  static bool isTablet(Size size) {
    return size.width >= mobileBreakpoint && size.width < tabletBreakpoint;
  }
  
  /// Check if the device is in desktop form factor
  static bool isDesktop(Size size) {
    return size.width >= desktopBreakpoint;
  }
  
  /// Get appropriate column count based on screen size
  static int getColumnCount(Size size) {
    if (isSteamDeckResolution(size)) return 2; // Optimized for Steam Deck
    if (isMobile(size)) return 1;
    if (isTablet(size)) return 2;
    return 3; // Desktop
  }
  
  /// Get appropriate padding based on screen size
  static EdgeInsets getScreenPadding(Size size) {
    if (isSteamDeckResolution(size)) {
      return const EdgeInsets.all(12.0); // Comfortable for gaming mode
    }
    if (isMobile(size)) return const EdgeInsets.all(16.0);
    if (isTablet(size)) return const EdgeInsets.all(24.0);
    return const EdgeInsets.all(32.0); // Desktop
  }
  
  /// Get appropriate spacing based on screen size
  static double getSpacing(Size size) {
    if (isSteamDeckResolution(size)) return 12.0; // Tight spacing for Steam Deck
    if (isMobile(size)) return 16.0;
    if (isTablet(size)) return 20.0;
    return 24.0; // Desktop
  }
}

/// Steam Deck specific theme adjustments
class SteamDeckTheme {
  /// Get Steam Deck optimized theme based on base theme
  static ThemeData getSteamDeckTheme(ThemeData baseTheme) {
    return baseTheme.copyWith(
      // Larger touch targets for finger/gamepad navigation
      materialTapTargetSize: MaterialTapTargetSize.padded,
      
      // Enhanced button themes for gamepad navigation
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: baseTheme.elevatedButtonTheme.style?.copyWith(
          padding: WidgetStateProperty.all(
            const EdgeInsets.symmetric(horizontal: 32, vertical: 16), // Larger buttons
          ),
          textStyle: WidgetStateProperty.all(
            baseTheme.textTheme.bodyLarge?.copyWith(
              fontSize: 16, // Larger text for readability
            ),
          ),
        ),
      ),
      
      // Steam Deck optimized card theme
      cardTheme: baseTheme.cardTheme.copyWith(
        margin: const EdgeInsets.all(8.0), // Comfortable spacing
        elevation: 4, // More pronounced shadows for depth
      ),
      
      // Enhanced list tile theme for better touch targets
      listTileTheme: baseTheme.listTileTheme.copyWith(
        contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
        minVerticalPadding: 12, // Ensure good touch targets
      ),
      
      // Steam Deck specific app bar adjustments
      appBarTheme: baseTheme.appBarTheme.copyWith(
        toolbarHeight: 64, // Slightly taller for better touch access
        titleTextStyle: baseTheme.textTheme.titleLarge?.copyWith(
          fontSize: 22, // Larger title text
        ),
      ),
    );
  }
  
  /// Steam Deck gaming mode colors (darker theme optimized for OLED)
  static const Color steamDeckBlue = Color(0xFF1A9FFF);
  static const Color steamDeckGray = Color(0xFF1E2328);
  static const Color steamDeckDarkGray = Color(0xFF16181D);
  static const Color steamDeckAccent = Color(0xFF00D4AA);
  
  /// Get Steam Deck specific dark theme
  static ThemeData get steamDeckGamingTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.dark,
      colorScheme: const ColorScheme.dark(
        primary: steamDeckBlue,
        secondary: steamDeckAccent,
        surface: steamDeckGray,
        onSurface: Colors.white,
        background: steamDeckDarkGray,
        onBackground: Colors.white,
      ),
      // Apply Steam Deck optimizations to the dark theme
    );
  }
}