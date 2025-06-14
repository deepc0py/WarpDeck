#!/bin/bash

# WarpDeck Development Environment Setup Script
# This script sets up the development environment for building WarpDeck

set -e

echo "🚀 Setting up WarpDeck development environment..."

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
    echo "📱 Detected macOS"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
    echo "🐧 Detected Linux"
else
    echo "❌ Unsupported operating system: $OSTYPE"
    exit 1
fi

# Check for Flutter
if ! command -v flutter &> /dev/null; then
    echo "❌ Flutter is not installed. Please install Flutter 3.22.2+ first:"
    echo "   https://docs.flutter.dev/get-started/install"
    exit 1
else
    echo "✅ Flutter found: $(flutter --version | head -n1)"
fi

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake is not installed. Please install CMake 3.15+ first"
    exit 1
else
    echo "✅ CMake found: $(cmake --version | head -n1)"
fi

# Platform-specific setup
if [[ "$OS" == "macOS" ]]; then
    echo "🍎 Setting up macOS dependencies..."
    
    # Check for Homebrew
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew is not installed. Please install Homebrew first:"
        echo "   https://brew.sh"
        exit 1
    fi
    
    # Install dependencies
    echo "📦 Installing Homebrew dependencies..."
    brew install openssl brotli pkg-config
    
    # Check for Xcode Command Line Tools
    if ! xcode-select -p &> /dev/null; then
        echo "❌ Xcode Command Line Tools not found. Installing..."
        xcode-select --install
        echo "⏳ Please complete the Xcode Command Line Tools installation and re-run this script"
        exit 1
    fi
    
elif [[ "$OS" == "Linux" ]]; then
    echo "🐧 Setting up Linux dependencies..."
    
    # Detect package manager and install dependencies
    if command -v apt-get &> /dev/null; then
        echo "📦 Installing apt dependencies..."
        sudo apt-get update
        sudo apt-get install -y \
            cmake \
            build-essential \
            pkg-config \
            libssl-dev \
            libavahi-client-dev \
            libgtk-3-dev \
            ninja-build \
            clang \
            libayatana-appindicator3-dev
    elif command -v dnf &> /dev/null; then
        echo "📦 Installing dnf dependencies..."
        sudo dnf install -y \
            cmake \
            gcc-c++ \
            pkg-config \
            openssl-devel \
            avahi-devel \
            gtk3-devel \
            ninja-build \
            clang \
            libayatana-appindicator3-devel
    elif command -v pacman &> /dev/null; then
        echo "📦 Installing pacman dependencies..."
        sudo pacman -S --needed \
            cmake \
            base-devel \
            pkg-config \
            openssl \
            avahi \
            gtk3 \
            ninja \
            clang \
            libayatana-appindicator
    else
        echo "❌ Unsupported package manager. Please install dependencies manually:"
        echo "   cmake, build-essential, pkg-config, libssl-dev, libavahi-client-dev, libgtk-3-dev, ninja-build, clang, libayatana-appindicator3-dev"
        exit 1
    fi
fi

# Set up vcpkg
echo "🔧 Setting up vcpkg for C++ dependencies..."
if [ ! -d "vcpkg" ]; then
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    if [[ "$OS" == "macOS" ]]; then
        ./bootstrap-vcpkg.sh
    else
        ./bootstrap-vcpkg.sh
    fi
    ./vcpkg integrate install
    cd ..
else
    echo "✅ vcpkg already exists"
fi

# Install C++ dependencies
echo "📦 Installing C++ dependencies via vcpkg..."
cd vcpkg
./vcpkg install boost-asio openssl nlohmann-json
cd ..

# Set up Flutter project
echo "🎨 Setting up Flutter project..."
cd warpdeck-flutter/warpdeck_gui
flutter pub get

# Generate FFI bindings if they exist
if [ -f "build.yaml" ]; then
    echo "🔗 Generating FFI bindings..."
    dart run build_runner build --delete-conflicting-outputs
fi

cd ../..

echo ""
echo "✅ Development environment setup complete!"
echo ""
echo "🛠️  To build the project:"
echo "   1. Build libwarpdeck: cd libwarpdeck && mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake .. && make"
echo "   2. Build CLI: cd cli && mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake .. && make"
if [[ "$OS" == "macOS" ]]; then
    echo "   3. Build GUI: cd warpdeck-flutter/warpdeck_gui && flutter build macos"
else
    echo "   3. Build GUI: cd warpdeck-flutter/warpdeck_gui && flutter build linux"
fi
echo ""
echo "📖 See README.md for detailed build instructions"