#!/bin/bash

# WarpDeck macOS Production Build Script
# This script builds and packages WarpDeck for macOS distribution

set -e

echo "🚀 Building WarpDeck for macOS..."

# Clean previous builds
echo "🧹 Cleaning previous builds..."
flutter clean
flutter pub get

# Generate code if needed
echo "🔧 Generating code..."
dart run build_runner build --delete-conflicting-outputs

# Build for macOS release
echo "🏗️ Building Flutter app for macOS..."
flutter build macos --release --verbose

# Create app bundle structure
BUILD_DIR="build/macos/Build/Products/Release"
APP_NAME="WarpDeck.app"
APP_BUNDLE="$BUILD_DIR/$APP_NAME"

echo "📦 Creating production app bundle..."

# Copy libwarpdeck to the app bundle
if [ -f "../../libwarpdeck/build/libwarpdeck.dylib" ]; then
    echo "📚 Copying libwarpdeck.dylib to app bundle..."
    mkdir -p "$APP_BUNDLE/Contents/Frameworks"
    cp "../../libwarpdeck/build/libwarpdeck.dylib" "$APP_BUNDLE/Contents/Frameworks/"
    
    # Update the library path in the executable
    install_name_tool -change \
        "libwarpdeck.dylib" \
        "@executable_path/../Frameworks/libwarpdeck.dylib" \
        "$APP_BUNDLE/Contents/MacOS/warpdeck_gui"
else
    echo "⚠️ Warning: libwarpdeck.dylib not found. Make sure to build libwarpdeck first."
fi

# Create DMG installer
echo "💿 Creating DMG installer..."
DMG_NAME="WarpDeck-v1.0.0-macOS"
hdiutil create -volname "WarpDeck" -srcfolder "$APP_BUNDLE" -ov -format UDZO "$BUILD_DIR/$DMG_NAME.dmg"

echo "✅ macOS build complete!"
echo "📍 App bundle: $APP_BUNDLE"
echo "📍 DMG installer: $BUILD_DIR/$DMG_NAME.dmg"

# Display app info
echo ""
echo "📋 Build Information:"
echo "   App Name: WarpDeck"
echo "   Version: 1.0.0"
echo "   Platform: macOS"
echo "   Architecture: Universal (ARM64 + x86_64)"
echo "   Bundle ID: com.warpdeck.gui"