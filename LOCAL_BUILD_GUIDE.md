# Local Build Guide for WarpDeck

This guide shows you how to build WarpDeck locally to test before pushing to GitHub.

## Prerequisites

### 1. System Dependencies

**On macOS:**
```bash
# Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake openssl brotli pkg-config flutter
```

**On Ubuntu/Debian:**
```bash
# Install system dependencies
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
    libayatana-appindicator3-dev \
    libayatana-ido3-0.4-dev \
    librsvg2-bin \
    imagemagick \
    curl \
    file \
    git \
    unzip \
    xz-utils \
    zip \
    libglu1-mesa

# Install Flutter
snap install flutter --classic
# OR download from https://docs.flutter.dev/get-started/install/linux
```

### 2. Flutter Setup

```bash
# Verify Flutter installation
flutter doctor

# Make sure you have Flutter 3.24.0 or later
flutter --version

# If you need to upgrade Flutter
flutter upgrade
```

### 3. vcpkg Setup

```bash
# Clone vcpkg (if not already done)
git clone https://github.com/Microsoft/vcpkg.git /usr/local/share/vcpkg

# Bootstrap vcpkg
cd /usr/local/share/vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
# ./bootstrap-vcpkg.bat  # Windows

# Set environment variable
export VCPKG_ROOT=/usr/local/share/vcpkg
echo 'export VCPKG_ROOT=/usr/local/share/vcpkg' >> ~/.bashrc
```

## Build Process

### Step 1: Build libwarpdeck (Native Library)

```bash
cd /Users/jesse/code/WarpDeck/libwarpdeck

# Install C++ dependencies
$VCPKG_ROOT/vcpkg install boost-asio openssl nlohmann-json

# Clean and build
rm -rf build
mkdir -p build && cd build

# Configure with vcpkg
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      ..

# Build (use appropriate number of cores)
make -j$(nproc)  # Linux
# make -j$(sysctl -n hw.ncpu)  # macOS

# Verify the library was built
ls -la libwarpdeck.*
```

### Step 2: Build Flutter App

```bash
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui

# Clean previous builds
flutter clean

# Get dependencies
flutter pub get

# Verify header files are available for FFI generation
ls -la ../../libwarpdeck/include/

# Run code generation
dart run build_runner build --delete-conflicting-outputs --verbose

# Run static analysis (optional but recommended)
dart analyze

# Build for your platform
flutter build linux --release --verbose   # Linux
flutter build macos --release --verbose   # macOS
flutter build windows --release --verbose # Windows
```

### Step 3: Copy Native Library (Important!)

After building, you need to copy the native library to the Flutter bundle:

**Linux:**
```bash
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui

# Copy libwarpdeck.so to Flutter bundle
mkdir -p build/linux/x64/release/bundle/lib
cp ../../libwarpdeck/build/libwarpdeck.so build/linux/x64/release/bundle/lib/

# Test the app
./build/linux/x64/release/bundle/warpdeck_gui
```

**macOS:**
```bash
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui

# Copy libwarpdeck.dylib to Flutter bundle
mkdir -p build/macos/Build/Products/Release/warpdeck_gui.app/Contents/Frameworks
cp ../../libwarpdeck/build/libwarpdeck.dylib build/macos/Build/Products/Release/warpdeck_gui.app/Contents/Frameworks/

# Test the app
open build/macos/Build/Products/Release/warpdeck_gui.app
```

## Using the Build Scripts

There are pre-made build scripts that handle everything:

### Linux Build
```bash
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui

# Make sure libwarpdeck is built first
cd ../../libwarpdeck && mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)
cd ../../warpdeck-flutter/warpdeck_gui

# Run the Linux build script
chmod +x build_scripts/build_linux.sh
./build_scripts/build_linux.sh
```

### macOS Build
```bash
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui

# Make sure libwarpdeck is built first
cd ../../libwarpdeck && mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ..
make -j$(sysctl -n hw.ncpu)
cd ../../warpdeck-flutter/warpdeck_gui

# Run the macOS build script
chmod +x build_scripts/build_macos.sh
./build_scripts/build_macos.sh
```

## Quick Development Build

For faster development builds (without packaging):

```bash
# Build libwarpdeck (do this once)
cd /Users/jesse/code/WarpDeck/libwarpdeck
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)

# Build Flutter app
cd ../../warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter build linux --debug  # Much faster than --release
```

## Troubleshooting

### Common Issues

1. **"No such file or directory: libwarpdeck.so"**
   - Make sure you built libwarpdeck first
   - Check that the library exists: `ls -la ../../libwarpdeck/build/libwarpdeck.*`

2. **"Failed to load libwarpdeck"**
   - The library needs to be in the Flutter bundle
   - Follow the "Copy Native Library" step above

3. **"dart run build_runner build" fails**
   - Make sure header files exist: `ls -la ../../libwarpdeck/include/`
   - Try cleaning: `flutter clean && flutter pub get`

4. **vcpkg issues**
   - Make sure VCPKG_ROOT is set correctly
   - Try rebuilding vcpkg: `cd $VCPKG_ROOT && ./vcpkg install boost-asio openssl nlohmann-json`

### Debug Commands

```bash
# Check Flutter doctor
flutter doctor -v

# Check what libraries are available
ls -la /Users/jesse/code/WarpDeck/libwarpdeck/build/

# Check Flutter build output
flutter build linux --release --verbose

# Test library loading
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui
dart run lib/services/libwarpdeck_ffi.dart
```

## Testing Before Push

To exactly replicate the GitHub Actions build:

```bash
# Use the same Flutter version as CI
flutter --version  # Should be 3.24.0+

# Follow the exact steps from the workflow
cd /Users/jesse/code/WarpDeck/warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs --verbose
dart analyze
flutter build linux --release

# Check that everything works
./build/linux/x64/release/bundle/warpdeck_gui
```

This should catch most issues before pushing to GitHub!