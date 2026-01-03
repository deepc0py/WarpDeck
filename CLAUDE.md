# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Initial Setup
```bash
./setup-dev.sh  # Installs all dependencies and sets up vcpkg
```

### Building libwarpdeck (C++ core library)
```bash
cd libwarpdeck
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)
```

### Building the CLI
```bash
cd warpdeck-cli
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)
```

### Building the Flutter GUI

**macOS:**
```bash
cd warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter build macos --release
```

**Linux:**
```bash
cd warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter build linux --release
```

### Running Flutter in Debug Mode
```bash
cd warpdeck-flutter/warpdeck_gui
flutter run -d macos   # or -d linux
```

### Analyze Flutter Code
```bash
cd warpdeck-flutter/warpdeck_gui
flutter analyze
```

## Architecture Overview

WarpDeck is a peer-to-peer file sharing application with three main components:

### libwarpdeck (C++ Core Library)
Location: `libwarpdeck/`

A C++17 static/shared library providing all core functionality:
- **mDNS/DNS-SD discovery**: Uses mjansson/mdns for cross-platform peer discovery via `_warpdeck._tcp.local.` service
- **HTTPS REST API**: cpp-httplib-based server/client for secure file transfers over TLS 1.3
- **Transfer management**: File I/O, progress tracking, session management
- **Security**: OpenSSL for TLS, Trust-On-First-Use (TOFU) certificate model

Key source files:
- `src/mdns_manager.cpp`: mDNS service registration and browsing
- `src/api_server.cpp`: Embedded HTTPS REST server
- `src/api_client.cpp`: HTTPS client for outgoing transfers
- `src/transfer_manager.cpp`: File transfer state machine
- `src/security_manager.cpp`: Certificate generation, trust store

Public API exposed via C-style FFI in `include/warpdeck.h`.

### warpdeck-cli (C++ CLI)
Location: `warpdeck-cli/`

Command-line interface linking against libwarpdeck:
- `warpdeck listen` - Daemon mode, broadcasts presence and receives files
- `warpdeck list` - One-shot peer discovery
- `warpdeck send --to <device_id> <files...>` - Send files to a peer
- `warpdeck config --set-name <name>` - Set device name

### warpdeck_gui (Flutter Desktop App)
Location: `warpdeck-flutter/warpdeck_gui/`

Cross-platform GUI using Flutter with dart:ffi bindings to libwarpdeck:
- `lib/services/libwarpdeck_ffi.dart`: FFI bindings to libwarpdeck.dylib/.so
- `lib/services/warpdeck_service.dart`: High-level Dart wrapper around FFI
- `lib/screens/`: Dashboard, Settings, Debug screens
- `lib/models/`: Peer, Transfer data models with json_serializable
- `lib/widgets/`: Reusable UI components

State management: Riverpod
Desktop integration: window_manager, system_tray, launch_at_startup

## Development Workflow

### Branch Strategy
- Create branches for each task: `feature/`, `fix/`, `docs/`
- Always commit and create PRs for completed work
- Builds are triggered by merged PRs via GitHub Actions

### Issue Management
- Create GitHub issues for substantial tasks (features, bug investigations)
- Do NOT create issues for small fixes or minor changes
- Reference issues in PR descriptions (e.g., "Resolves #123")

### CI/CD
- `.github/workflows/build.yml`: Runs on PRs, builds all components
- `.github/workflows/release.yml`: Creates releases on main branch merges
- Cross-platform: macOS and Linux builds

## Platform Notes

### macOS
- Uses vcpkg for C++ dependencies
- Requires Xcode Command Line Tools
- App sandboxed with security-scoped bookmarks for file access

### Linux (including Steam Deck)
- Requires avahi-client for mDNS
- Packaged as Flatpak or AppImage
- Gaming Mode UI (10-foot interface) detected via gamescope session

## Key Dependencies

**C++ (via vcpkg/FetchContent):**
- OpenSSL (TLS, crypto)
- cpp-httplib (HTTP server/client)
- nlohmann/json (JSON parsing)
- mjansson/mdns (mDNS discovery)

**Flutter:**
- flutter_riverpod (state management)
- ffi (native code integration)
- file_picker, path_provider (file system)
- window_manager, system_tray (desktop integration)
