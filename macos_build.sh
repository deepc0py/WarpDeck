#!/bin/bash

# macOS-specific build script using Homebrew instead of vcpkg
# This avoids the vcpkg OpenSSL ARM64 issues

set -e

echo "🍎 Building WarpDeck on macOS (using Homebrew)..."

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is for macOS only. Use test_local_build.sh for Linux."
    exit 1
fi

# Check if we're in the right directory
if [ ! -f "libwarpdeck/CMakeLists.txt" ]; then
    echo "❌ Please run this script from the WarpDeck root directory"
    exit 1
fi

# Install dependencies via Homebrew
echo "🍺 Installing dependencies via Homebrew..."
brew install cmake pkg-config openssl nlohmann-json boost flutter

# Check Flutter version
echo "📱 Checking Flutter version..."
flutter --version

# Build libwarpdeck without vcpkg
echo "🔨 Building libwarpdeck..."
cd libwarpdeck

# Clean and build
rm -rf build
mkdir -p build && cd build

echo "⚙️ Configuring cmake (Homebrew-only)..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
      -DOPENSSL_LIBRARIES=$(brew --prefix openssl)/lib \
      -DOPENSSL_INCLUDE_DIR=$(brew --prefix openssl)/include \
      -DBoost_ROOT=$(brew --prefix boost) \
      -Dnlohmann_json_DIR=$(brew --prefix nlohmann-json)/lib/cmake/nlohmann_json \
      -DCMAKE_PREFIX_PATH="$(brew --prefix openssl);$(brew --prefix boost);$(brew --prefix nlohmann-json)" \
      ..

echo "🏗️ Building libwarpdeck..."
make -j$(sysctl -n hw.ncpu)

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
dart analyze

echo "🏗️ Building Flutter macOS app..."
flutter build macos --release --verbose

echo "📚 Copying libwarpdeck.dylib to app bundle..."
mkdir -p build/macos/Build/Products/Release/warpdeck_gui.app/Contents/Frameworks
cp ../../libwarpdeck/build/libwarpdeck.dylib build/macos/Build/Products/Release/warpdeck_gui.app/Contents/Frameworks/

echo ""
echo "🎉 macOS build completed successfully!"
echo ""
echo "🚀 To run the app:"
echo "   open build/macos/Build/Products/Release/warpdeck_gui.app"
echo ""
echo "📦 App bundle location:"
echo "   $(pwd)/build/macos/Build/Products/Release/warpdeck_gui.app"
echo ""