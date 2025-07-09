#!/bin/bash

# Quick local build test script for WarpDeck
# This replicates the GitHub Actions workflow locally

set -e

echo "🧪 Testing WarpDeck local build environment..."

# Check Flutter version
echo "📱 Checking Flutter version..."
flutter --version
FLUTTER_VERSION=$(flutter --version | head -1 | cut -d' ' -f2)
echo "Flutter version: $FLUTTER_VERSION"

# Check required tools
echo "🔧 Checking required tools..."
command -v cmake >/dev/null 2>&1 || { echo "❌ cmake not found"; exit 1; }
command -v make >/dev/null 2>&1 || { echo "❌ make not found"; exit 1; }
command -v dart >/dev/null 2>&1 || { echo "❌ dart not found"; exit 1; }

echo "✅ Basic tools found"

# Check vcpkg
echo "🏗️ Checking vcpkg..."
if [ -z "$VCPKG_ROOT" ]; then
    echo "❌ VCPKG_ROOT not set. Please set it to your vcpkg installation."
    echo "   export VCPKG_ROOT=/path/to/vcpkg"
    exit 1
fi

if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
    echo "❌ vcpkg not found at $VCPKG_ROOT"
    exit 1
fi

echo "✅ vcpkg found at $VCPKG_ROOT"

# Check project structure
echo "📁 Checking project structure..."
if [ ! -f "libwarpdeck/CMakeLists.txt" ]; then
    echo "❌ libwarpdeck/CMakeLists.txt not found. Are you in the WarpDeck root directory?"
    exit 1
fi

if [ ! -f "warpdeck-flutter/warpdeck_gui/pubspec.yaml" ]; then
    echo "❌ Flutter project not found"
    exit 1
fi

echo "✅ Project structure looks good"

# Build libwarpdeck
echo "🔨 Building libwarpdeck..."
cd libwarpdeck

# Install dependencies
echo "📦 Installing C++ dependencies..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "🍺 Using Homebrew OpenSSL on macOS (faster than vcpkg)"
    brew install openssl nlohmann-json
    $VCPKG_ROOT/vcpkg install boost-asio
else
    $VCPKG_ROOT/vcpkg install boost-asio openssl nlohmann-json
fi

# Clean and build
rm -rf build
mkdir -p build && cd build

echo "⚙️ Configuring cmake..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # Use Homebrew OpenSSL on macOS
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
          -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
          -DOPENSSL_LIBRARIES=$(brew --prefix openssl)/lib \
          -DOPENSSL_INCLUDE_DIR=$(brew --prefix openssl)/include \
          ..
else
    # Use vcpkg OpenSSL on Linux
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
          ..
fi

echo "🏗️ Building libwarpdeck..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    make -j$(sysctl -n hw.ncpu)
else
    make -j$(nproc)
fi

echo "✅ libwarpdeck built successfully"

# Check library files
echo "📚 Checking library files..."
ls -la libwarpdeck*

cd ../..

# Build Flutter app
echo "📱 Building Flutter app..."
cd warpdeck-flutter/warpdeck_gui

echo "📦 Getting Flutter dependencies..."
flutter pub get

echo "🔍 Checking header files..."
ls -la ../../libwarpdeck/include/

echo "🔧 Running code generation..."
dart run build_runner build --delete-conflicting-outputs --verbose

echo "🔍 Running static analysis..."
dart analyze --fatal-warnings

echo "🏗️ Building Flutter app..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    flutter build macos --release --verbose
    echo "📱 macOS app built successfully!"
    echo "🎯 App location: build/macos/Build/Products/Release/warpdeck_gui.app"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    flutter build linux --release --verbose
    echo "📱 Linux app built successfully!"
    echo "🎯 App location: build/linux/x64/release/bundle/warpdeck_gui"
    
    # Copy library to bundle
    echo "📚 Copying libwarpdeck.so to bundle..."
    mkdir -p build/linux/x64/release/bundle/lib
    cp ../../libwarpdeck/build/libwarpdeck.so build/linux/x64/release/bundle/lib/
    echo "✅ Library copied to bundle"
else
    echo "❌ Unsupported OS: $OSTYPE"
    exit 1
fi

echo ""
echo "🎉 Local build completed successfully!"
echo ""
echo "Next steps:"
echo "1. Test the app by running it"
echo "2. If it works, your changes should work on GitHub Actions too"
echo "3. If it fails, debug locally before pushing"
echo ""
echo "Happy coding! 🚀"