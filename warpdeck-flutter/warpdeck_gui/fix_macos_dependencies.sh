#!/bin/bash

# Script to fix macOS WarpDeck dependencies for distribution
# This bundles all required libraries with the app bundle

set -e

APP_BUNDLE="/Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui/build/macos/Build/Products/Release/warpdeck_gui.app"
MACOS_DIR="$APP_BUNDLE/Contents/MacOS"
DYLIB_PATH="$MACOS_DIR/libwarpdeck.dylib"

echo "🔧 Fixing WarpDeck macOS dependencies..."

# Check if app bundle exists
if [ ! -d "$APP_BUNDLE" ]; then
    echo "❌ App bundle not found. Please run 'flutter build macos --release' first."
    exit 1
fi

# Copy the latest libwarpdeck.dylib
echo "📦 Copying libwarpdeck.dylib..."
cp "/Users/jesse/code/WarpDeck/libwarpdeck/build/libwarpdeck.dylib" "$DYLIB_PATH"

# Copy OpenSSL dependencies
echo "📦 Copying OpenSSL libraries..."
chmod +w "$MACOS_DIR/libssl.3.dylib" 2>/dev/null || true
chmod +w "$MACOS_DIR/libcrypto.3.dylib" 2>/dev/null || true
cp "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib" "$MACOS_DIR/"
cp "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib" "$MACOS_DIR/"

# Copy Brotli dependencies  
echo "📦 Copying Brotli libraries..."
chmod +w "$MACOS_DIR/libbrotli"*.dylib 2>/dev/null || true
cp "/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib" "$MACOS_DIR/"
cp "/opt/homebrew/opt/brotli/lib/libbrotlienc.1.dylib" "$MACOS_DIR/"
cp "/opt/homebrew/opt/brotli/lib/libbrotlidec.1.dylib" "$MACOS_DIR/"

# Fix library paths in libwarpdeck.dylib
echo "🔧 Fixing library paths in libwarpdeck.dylib..."
install_name_tool -change "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib" "@loader_path/libssl.3.dylib" "$DYLIB_PATH"
install_name_tool -change "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib" "@loader_path/libcrypto.3.dylib" "$DYLIB_PATH"
install_name_tool -change "/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib" "@loader_path/libbrotlicommon.1.dylib" "$DYLIB_PATH"
install_name_tool -change "/opt/homebrew/opt/brotli/lib/libbrotlienc.1.dylib" "@loader_path/libbrotlienc.1.dylib" "$DYLIB_PATH"
install_name_tool -change "/opt/homebrew/opt/brotli/lib/libbrotlidec.1.dylib" "@loader_path/libbrotlidec.1.dylib" "$DYLIB_PATH"

# Fix internal dependencies in bundled libraries
echo "🔧 Fixing internal dependency paths..."

# Fix OpenSSL crypto reference in ssl library
SSL_DYLIB="$MACOS_DIR/libssl.3.dylib"
if [ -f "$SSL_DYLIB" ]; then
    chmod +w "$SSL_DYLIB"
    # Check what crypto path it's using and fix it
    CRYPTO_PATH=$(otool -L "$SSL_DYLIB" | grep libcrypto | awk '{print $1}' | head -1)
    if [[ "$CRYPTO_PATH" == */opt/homebrew/* ]]; then
        install_name_tool -change "$CRYPTO_PATH" "@loader_path/libcrypto.3.dylib" "$SSL_DYLIB"
    fi
fi

# Fix Brotli cross-dependencies
BROTLI_ENC="$MACOS_DIR/libbrotlienc.1.dylib"
BROTLI_DEC="$MACOS_DIR/libbrotlidec.1.dylib"

if [ -f "$BROTLI_ENC" ]; then
    chmod +w "$BROTLI_ENC"
    COMMON_PATH=$(otool -L "$BROTLI_ENC" | grep libbrotlicommon | awk '{print $1}' | head -1)
    if [[ "$COMMON_PATH" == */opt/homebrew/* ]]; then
        install_name_tool -change "$COMMON_PATH" "@loader_path/libbrotlicommon.1.dylib" "$BROTLI_ENC"
    fi
fi

if [ -f "$BROTLI_DEC" ]; then
    chmod +w "$BROTLI_DEC"
    COMMON_PATH=$(otool -L "$BROTLI_DEC" | grep libbrotlicommon | awk '{print $1}' | head -1)
    if [[ "$COMMON_PATH" == */opt/homebrew/* ]]; then
        install_name_tool -change "$COMMON_PATH" "@loader_path/libbrotlicommon.1.dylib" "$BROTLI_DEC"
    fi
fi

# Re-sign the app to fix code signature issues after modifying libraries
echo "🔏 Re-signing app bundle..."
codesign --force --deep --sign - "$APP_BUNDLE"

echo "✅ Dependencies fixed and app re-signed! WarpDeck should now run without library loading errors."
echo ""
echo "To test, run:"
echo "  open '$APP_BUNDLE'"
echo ""
echo "The error status indicator should now be clickable and show meaningful error messages."