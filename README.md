# WarpDeck

**AirDrop for the rest of us.** Cross-platform, peer-to-peer file sharing between macOS, Linux, and Steam Deck. No cloud. No accounts. No setup.

[![Build and Release](https://github.com/deepc0py/WarpDeck/actions/workflows/release.yml/badge.svg)](https://github.com/deepc0py/WarpDeck/actions/workflows/release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Steam%20Deck-blue)]()

---

WarpDeck discovers nearby devices automatically via mDNS, encrypts every transfer with TLS 1.3, and remembers trusted devices using a Trust-On-First-Use model (like SSH). Drop files onto a device name and they arrive on the other side. That's it.

## Features

- **Zero-configuration discovery** -- mDNS/DNS-SD (`_warpdeck._tcp.local.`) finds peers automatically on your LAN
- **End-to-end encrypted** -- TLS 1.3 for every transfer. No plaintext, ever.
- **Trust-On-First-Use security** -- Approve a device once, it's trusted forever. No passwords. No accounts.
- **Cross-platform** -- macOS, Linux, and Steam Deck with a dedicated 10-foot Gaming Mode UI
- **Fast** -- Up to 1.9 GB/s LAN throughput with streaming I/O (files never fully buffered in memory)
- **Desktop GUI + CLI** -- Flutter desktop app for everyday use, full CLI for headless/server/scripting
- **Folders and queues** -- Send entire directory trees; transfers queue and process sequentially

## How It Works

```
   Sender                           LAN                          Receiver
     |                                                               |
     |  1. DISCOVER  (mDNS broadcast: "_warpdeck._tcp.local.")       |
     |-------------------------------------------------------------->|
     |                                                               |
     |  2. APPROVE   (first time only -- TOFU, like SSH)             |
     |<--------------------------------------------------------------|
     |               "Jesse's Mac wants to send files. Accept?"      |
     |                                                               |
     |  3. TRANSFER  (HTTPS/TLS 1.3 -- encrypted, streamed)         |
     |=============================================================>|
     |               game-mod.zip ===== 100% @ 450 MB/s             |
     |                                                               |
```

1. **Discover**: Launch WarpDeck on two devices on the same network. They find each other instantly via mDNS.
2. **Approve**: The first time two devices connect, the receiver sees an approval prompt. Accept once, and the device is permanently trusted (its certificate fingerprint is stored locally).
3. **Transfer**: Drag files onto the device name (GUI) or use the CLI. Files stream over TLS 1.3 directly between devices. No relay. No cloud.

## Architecture

WarpDeck is three layers: a C++ engine, a Flutter GUI, and a CLI. The core handles all networking, crypto, and I/O. The frontends are thin.

```
+----------------------------------------------------------+
|                    warpdeck-gui (Flutter/Dart)            |
|          Riverpod state / system tray / drag-drop        |
+------------------------------+---------------------------+
                               |  dart:ffi
+------------------------------v---------------------------+
|                     libwarpdeck (C++17)                   |
|                                                          |
|  +-------------+  +------------+  +-------------------+  |
|  | mDNS        |  | HTTPS REST |  | Transfer Manager  |  |
|  | Discovery   |  | Server +   |  | (state machine,   |  |
|  | (DNS-SD)    |  | Client     |  |  file I/O, queue) |  |
|  +-------------+  +------------+  +-------------------+  |
|  +-------------+  +------------------------------------+ |
|  | Security    |  | C-style FFI API (warpdeck.h)       | |
|  | Manager     |  | opaque handle, callbacks, JSON     | |
|  | (TLS, TOFU) |  +------------------------------------+ |
|  +-------------+                                         |
+----------------------------------------------------------+

+----------------------------------------------------------+
|                    warpdeck-cli (C++)                     |
|         listen / send / list / config                    |
+----------------------------------------------------------+
```

| Layer | Language | Role |
|-------|----------|------|
| **libwarpdeck** | C++17 | Core engine: mDNS discovery, HTTPS REST server/client, TLS certificates, TOFU trust store, transfer state machine. Exposes a C-style FFI API via `warpdeck.h`. |
| **warpdeck-gui** | Flutter/Dart | Desktop app with Riverpod state management, system tray integration, launch-at-startup. Communicates with libwarpdeck via `dart:ffi`. |
| **warpdeck-cli** | C++ | Command-line interface for headless use, automation, and power users. Links directly against libwarpdeck. |

## Security Model

| Property | Implementation |
|----------|---------------|
| Key generation | RSA-2048 on first launch |
| Certificates | Self-signed X.509 (no CA dependency) |
| Transport | TLS 1.3 only (older protocols disabled) |
| Authentication | Trust-On-First-Use with SHA-256 certificate fingerprinting |
| Approval timeout | 60 seconds |
| Network scope | Local network only -- no internet exposure |

## Installation

### Download

| Platform | Format | Link |
|----------|--------|------|
| **macOS** | DMG | [Latest Release](https://github.com/deepc0py/WarpDeck/releases/latest) |
| **Linux** | AppImage | [Latest Release](https://github.com/deepc0py/WarpDeck/releases/latest) |
| **Steam Deck** | AppImage | [Latest Release](https://github.com/deepc0py/WarpDeck/releases/latest) |

Builds are automatically produced from every merge to `main` via GitHub Actions.

### Building from Source

**Prerequisites**: CMake 3.15+, C++17 compiler, Flutter SDK 3.22+, OpenSSL

<details>
<summary><strong>macOS</strong></summary>

```bash
./setup-dev.sh

# Build the core library
cd libwarpdeck && mkdir -p build && cd build
cmake -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3) ..
make -j$(sysctl -n hw.ncpu)

# Build and run the GUI
cd ../../warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter run -d macos
```

</details>

<details>
<summary><strong>Linux</strong></summary>

```bash
./setup-dev.sh

# Build the core library
cd libwarpdeck && mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)

# Build and run the GUI
cd ../../warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
flutter run -d linux
```

</details>

## CLI Usage

```bash
warpdeck listen                           # Start daemon -- broadcast presence, receive files
warpdeck list                             # One-shot peer discovery
warpdeck send --to <device_id> file.txt   # Send files to a peer
warpdeck config --set-name "My Mac"       # Set device name
```

## Project Structure

```
WarpDeck/
├── libwarpdeck/              # C++17 core library
│   ├── include/              #   Public C API (warpdeck.h)
│   ├── src/                  #   Implementation
│   └── CMakeLists.txt
├── warpdeck-cli/             # Command-line interface
├── warpdeck-flutter/
│   └── warpdeck_gui/         # Flutter desktop app
├── third_party/              # Vendored deps (mjansson/mdns)
├── scripts/                  # Build & deploy scripts
└── .github/workflows/        # CI/CD
```

## Dependencies

**C++**: OpenSSL, [cpp-httplib](https://github.com/yhirose/cpp-httplib), [nlohmann/json](https://github.com/nlohmann/json), [mjansson/mdns](https://github.com/mjansson/mdns)

**Flutter**: flutter_riverpod, ffi, window_manager, system_tray, file_picker, google_fonts

## CI/CD

GitHub Actions builds on every push to `main`, targeting both macOS and Ubuntu. Releases are tagged automatically as `v{date}-{sha}` and produce macOS DMG, Linux AppImage, and CLI binaries.

## Steam Deck

WarpDeck includes a dedicated Gaming Mode UI built for the 10-foot experience:

- Large touch targets and gamepad-navigable focus system
- Steam color theme and controller button prompts
- Automatic detection of gamescope session to switch UI modes
- Works in both Desktop Mode (standard GUI) and Gaming Mode (controller UI)

## Contributing

1. Fork the repo
2. Run `./setup-dev.sh`
3. Create a feature branch
4. Make changes, test locally
5. Open a pull request

All PRs are built and tested automatically.

## License

WarpDeck is released under the MIT License.
