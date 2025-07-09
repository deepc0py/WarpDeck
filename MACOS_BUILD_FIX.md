# macOS ARM64 Build Fix

## Problem
The `test_local_build.sh` script fails on ARM64 macOS because vcpkg has issues building OpenSSL from source on Apple Silicon.

## Solution
Use the macOS-specific build script that uses Homebrew instead of vcpkg for OpenSSL.

## Quick Fix

### Option 1: Use macOS-specific script (Recommended)
```bash
./macos_build.sh
```

This script:
- Uses Homebrew for OpenSSL and other dependencies
- Avoids vcpkg OpenSSL compilation issues
- Is faster and more reliable on macOS

### Option 2: Fix vcpkg OpenSSL issue
If you want to use vcpkg, try updating it first:
```bash
cd $VCPKG_ROOT
git pull
./bootstrap-vcpkg.sh
./vcpkg update
```

Then try the original script:
```bash
./test_local_build.sh
```

### Option 3: Manual build (if scripts fail)
```bash
# Install dependencies
brew install cmake pkg-config openssl nlohmann-json boost flutter

# Build libwarpdeck
cd libwarpdeck
rm -rf build && mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
      -DOPENSSL_LIBRARIES=$(brew --prefix openssl)/lib \
      -DOPENSSL_INCLUDE_DIR=$(brew --prefix openssl)/include \
      ..
make -j$(sysctl -n hw.ncpu)
cd ../..

# Build Flutter app
cd warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter build macos --release

# Copy library to app bundle
mkdir -p build/macos/Build/Products/Release/warpdeck_gui.app/Contents/Frameworks
cp ../../libwarpdeck/build/libwarpdeck.dylib build/macos/Build/Products/Release/warpdeck_gui.app/Contents/Frameworks/
```

## Why This Happens
- vcpkg tries to build OpenSSL from source on ARM64 macOS
- The build process sometimes fails due to ARM64-specific compilation issues
- Homebrew provides pre-compiled ARM64 binaries that work better

## Expected Result
After using the macOS build script, you should see:
```
🎉 macOS build completed successfully!

🚀 To run the app:
   open build/macos/Build/Products/Release/warpdeck_gui.app
```

## Testing
To test the app:
```bash
open warpdeck-flutter/warpdeck_gui/build/macos/Build/Products/Release/warpdeck_gui.app
```

The app should launch without the hanging issue we fixed earlier.