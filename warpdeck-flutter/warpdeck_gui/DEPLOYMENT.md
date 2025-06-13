# WarpDeck Deployment Guide

## 🚀 Production Deployment

This guide covers building and deploying WarpDeck across all supported platforms.

## Prerequisites

### Development Environment
- **Flutter SDK**: 3.22.2 or later
- **Dart SDK**: 3.4.3 or later
- **Git**: For version control
- **Platform-specific tools** (see below)

### macOS Development
- **Xcode**: Latest version with command line tools
- **macOS**: 10.14+ (for deployment target)
- **Homebrew**: For dependency management

### Linux Development
- **Ubuntu/Debian**: 20.04+ recommended
- **Build tools**: `clang`, `cmake`, `ninja-build`
- **GTK3**: `libgtk-3-dev`
- **AppImage tools**: For packaging

## Building WarpDeck

### 1. Clone and Setup
```bash
git clone https://github.com/deepc0py/WarpDeck.git
cd WarpDeck/warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build --delete-conflicting-outputs
```

### 2. Build libwarpdeck (Required)
```bash
cd ../../libwarpdeck
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
```

## Platform-Specific Builds

### macOS Production Build
```bash
# Run the automated build script
chmod +x build_scripts/build_macos.sh
./build_scripts/build_macos.sh
```

**Output:**
- `build/macos/Build/Products/Release/WarpDeck.app` - Application bundle
- `build/macos/Build/Products/Release/WarpDeck-v1.0.0-macOS.dmg` - Installer

**Features:**
- ✅ Universal binary (ARM64 + x86_64)
- ✅ Code signing ready
- ✅ Notarization ready
- ✅ DMG installer with custom layout

### Linux Production Build
```bash
# Run the automated build script
chmod +x build_scripts/build_linux.sh
./build_scripts/build_linux.sh
```

**Output:**
- `build/linux/x64/release/bundle/` - Raw application
- `build/linux/WarpDeck.AppDir/` - AppImage structure
- `build/linux/com.warpdeck.GUI.yaml` - Flatpak manifest

**Formats:**
- ✅ AppImage (universal compatibility)
- ✅ Flatpak (modern Linux distributions)
- ✅ Raw bundle (custom packaging)

## Steam Deck Deployment

### Gaming Mode Integration
WarpDeck is optimized for Steam Deck with:
- **Gamepad navigation**: Full controller support
- **Gaming mode**: Steam integration
- **Performance**: Optimized for handheld gaming

### Installation Methods

#### Method 1: Desktop Mode (Recommended)
1. Switch to Desktop Mode
2. Download WarpDeck AppImage
3. Make executable: `chmod +x WarpDeck.AppImage`
4. Run: `./WarpDeck.AppImage`

#### Method 2: Steam Integration
1. Add as Non-Steam Game
2. Set launch options for controller support
3. Configure as utility in Steam library

### Steam Deck Optimizations
- **Display scaling**: Automatic DPI detection
- **Touch controls**: Optimized for 7" touchscreen
- **Battery efficiency**: Power management integration
- **Storage aware**: Handles SD card and internal storage

## Distribution Channels

### GitHub Releases
- **Automatic builds**: CI/CD pipeline creates releases
- **Asset uploads**: DMG, AppImage, and source packages
- **Version tagging**: Semantic versioning

### Platform Stores
- **Flathub**: Flatpak distribution for Linux
- **Homebrew**: macOS package manager (planned)
- **AUR**: Arch User Repository (community)

## CI/CD Pipeline

### GitHub Actions Workflow
Located: `.github/workflows/build.yml`

**Triggers:**
- Push to `main` branch
- Pull requests
- Version tags (`v*`)

**Build Matrix:**
- **macOS**: Latest runners, Xcode tools
- **Linux**: Ubuntu latest, GTK3, AppImage tools

**Artifacts:**
- macOS: `.app` bundle and `.dmg` installer
- Linux: AppImage and Flatpak files

## Installation Instructions

### macOS Users
1. Download `WarpDeck-v1.0.0-macOS.dmg`
2. Open DMG and drag WarpDeck to Applications
3. First launch: Right-click → Open (for unsigned builds)
4. Grant network permissions when prompted

### Linux Users

#### AppImage (Universal)
```bash
wget https://github.com/deepc0py/WarpDeck/releases/latest/download/WarpDeck.AppImage
chmod +x WarpDeck.AppImage
./WarpDeck.AppImage
```

#### Flatpak
```bash
flatpak install flathub com.warpdeck.GUI
flatpak run com.warpdeck.GUI
```

#### Package Managers
```bash
# Arch Linux (AUR)
yay -S warpdeck

# Ubuntu/Debian (planned)
sudo apt install warpdeck
```

### Steam Deck Users
1. Switch to Desktop Mode
2. Follow Linux AppImage instructions
3. Add to Steam as Non-Steam Game (optional)
4. Use Gaming Mode for touch-friendly interface

## Troubleshooting

### Common Issues

#### macOS Security Warnings
- **Issue**: "WarpDeck cannot be opened because it is from an unidentified developer"
- **Solution**: Right-click app → Open → Open anyway

#### Linux Permission Errors
- **Issue**: AppImage won't execute
- **Solution**: `chmod +x WarpDeck.AppImage`

#### Network Discovery Issues
- **Issue**: No peers discovered
- **Solution**: Check firewall settings, ensure local network access

#### Steam Deck Controller Issues
- **Issue**: Controller not working in Desktop Mode
- **Solution**: Launch from Steam or use touch controls

### Performance Optimization

#### System Requirements
- **RAM**: 512MB minimum, 1GB recommended
- **Storage**: 100MB application + transfer space
- **Network**: WiFi or Ethernet for peer discovery
- **OS**: macOS 10.14+, Linux with GTK3

#### Optimization Tips
- Close unused applications during large transfers
- Use SSD storage for better performance
- Ensure stable network connection
- Configure firewall exceptions

## Development Setup

### Running from Source
```bash
git clone https://github.com/deepc0py/WarpDeck.git
cd WarpDeck/warpdeck-flutter/warpdeck_gui
flutter pub get
dart run build_runner build
flutter run -d macos  # or -d linux
```

### Contributing
1. Fork the repository
2. Create feature branch
3. Test on target platforms
4. Submit pull request

## Support

### Getting Help
- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Community support and questions
- **Documentation**: Comprehensive guides and API docs

### Reporting Issues
Please include:
- Platform and version
- Error messages or logs
- Steps to reproduce
- Expected vs actual behavior

---

**Version**: 1.0.0  
**Last Updated**: 2024  
**Platforms**: macOS 10.14+, Linux (GTK3), Steam Deck  
**License**: MIT