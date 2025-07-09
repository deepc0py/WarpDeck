#!/bin/bash

# Quick development build script for WarpDeck
# This is faster than the full build and good for development

set -e

echo "🚀 Quick development build for WarpDeck..."

# Check if we're in the right directory
if [ ! -f "libwarpdeck/CMakeLists.txt" ]; then
    echo "❌ Please run this script from the WarpDeck root directory"
    exit 1
fi

# Check if libwarpdeck is already built
if [ ! -f "libwarpdeck/build/libwarpdeck.so" ] && [ ! -f "libwarpdeck/build/libwarpdeck.dylib" ]; then
    echo "🔨 Building libwarpdeck (first time)..."
    cd libwarpdeck
    mkdir -p build && cd build
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # Use Homebrew OpenSSL on macOS
        cmake -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
              -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
              -DOPENSSL_LIBRARIES=$(brew --prefix openssl)/lib \
              -DOPENSSL_INCLUDE_DIR=$(brew --prefix openssl)/include \
              ..
    else
        # Use vcpkg OpenSSL on Linux
        cmake -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
              ..
    fi
    if [[ "$OSTYPE" == "darwin"* ]]; then
        make -j$(sysctl -n hw.ncpu)
    else
        make -j$(nproc)
    fi
    cd ../..
    echo "✅ libwarpdeck built"
else
    echo "✅ libwarpdeck already built (skipping)"
fi

# Build Flutter app
echo "📱 Building Flutter app..."
cd warpdeck-flutter/warpdeck_gui

# Quick dependencies check
echo "📦 Checking dependencies..."
flutter pub get

# Only run code generation if needed
if [ ! -f "lib/models/peer.g.dart" ] || [ ! -f "lib/models/transfer.g.dart" ]; then
    echo "🔧 Running code generation..."
    dart run build_runner build --delete-conflicting-outputs
else
    echo "✅ Code generation up to date (skipping)"
fi

# Quick analysis (only errors)
echo "🔍 Quick analysis..."
dart analyze --fatal-infos

# Build for development (debug mode is faster)
echo "🏗️ Building Flutter app (debug mode)..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    flutter build macos --debug
    echo "📱 macOS debug app built!"
    echo "🎯 Run: open build/macos/Build/Products/Debug/warpdeck_gui.app"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    flutter build linux --debug
    echo "📱 Linux debug app built!"
    
    # Copy library to bundle
    mkdir -p build/linux/x64/debug/bundle/lib
    if [ -f "../../libwarpdeck/build/libwarpdeck.so" ]; then
        cp ../../libwarpdeck/build/libwarpdeck.so build/linux/x64/debug/bundle/lib/
    fi
    
    echo "🎯 Run: ./build/linux/x64/debug/bundle/warpdeck_gui"
fi

echo ""
echo "🎉 Development build completed!"
echo ""
echo "💡 Tips:"
echo "- Debug builds are faster but larger"
echo "- For release builds, use: flutter build [platform] --release"
echo "- For hot reload during development: flutter run -d linux (or macos)"
echo ""