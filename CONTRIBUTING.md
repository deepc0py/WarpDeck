# Contributing to WarpDeck 🤝

Thank you for considering contributing to WarpDeck! This document provides guidelines and information for contributors.

## 🚀 Quick Start

1. **Fork the repository** on GitHub
2. **Clone your fork** locally
3. **Set up the development environment**:
   ```bash
   ./setup-dev.sh
   ```
4. **Create a feature branch** from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
5. **Make your changes** and test them
6. **Submit a pull request**

## 🏗️ Development Setup

### Prerequisites

- **Flutter SDK** 3.22.2+ ([Install Flutter](https://docs.flutter.dev/get-started/install))
- **CMake** 3.15+ 
- **C++17** compatible compiler
- **Platform-specific tools**:
  - **macOS**: Xcode Command Line Tools, Homebrew
  - **Linux**: GTK3 development libraries, build-essential

### Quick Setup

Run the automated setup script:
```bash
./setup-dev.sh
```

This will install all necessary dependencies and set up the build environment.

### Manual Setup

If you prefer to set up manually, see the [Building from Source](README.md#building-from-source) section in the README.

## 🧪 Testing

### Running Tests

Our CI/CD pipeline automatically runs tests on every pull request. To run tests locally:

```bash
# Test Flutter code
cd warpdeck-flutter/warpdeck_gui
flutter test
dart analyze

# Test C++ code (if tests exist)
cd libwarpdeck/build
make test
```

### Quality Checks

Before submitting a PR, ensure your code passes quality checks:

```bash
# Format Dart code
cd warpdeck-flutter/warpdeck_gui
dart format .

# Check for analysis issues
dart analyze --fatal-infos

# Build to ensure everything compiles
flutter build linux --release  # or macos
```

## 📝 Code Style

### Dart/Flutter Code

- Follow [Dart style guidelines](https://dart.dev/guides/language/effective-dart/style)
- Use `dart format` to automatically format code
- Run `dart analyze` to check for issues
- Use meaningful variable and function names
- Add comments for complex logic

### C++ Code

- Follow modern C++17 standards
- Use clear, descriptive naming
- Keep functions small and focused
- Add documentation for public APIs
- Use RAII for resource management

## 🔄 Pull Request Process

1. **Ensure your PR**:
   - Has a clear, descriptive title
   - Includes a detailed description of changes
   - References any related issues
   - Passes all CI/CD checks

2. **PR Title Format**:
   ```
   type(scope): description
   
   Examples:
   feat(gui): add dark mode toggle
   fix(discovery): resolve mDNS crash on macOS
   docs(readme): update installation instructions
   ```

3. **Types**:
   - `feat`: New feature
   - `fix`: Bug fix
   - `docs`: Documentation changes
   - `style`: Code style changes
   - `refactor`: Code refactoring
   - `test`: Adding or updating tests
   - `chore`: Maintenance tasks

4. **CI/CD Checks**:
   Your PR must pass all automated checks:
   - ✅ Build succeeds on both macOS and Linux
   - ✅ All tests pass
   - ✅ Code analysis passes
   - ✅ Code is properly formatted

## 🐛 Bug Reports

When reporting bugs, please include:

- **OS and version** (macOS version, Linux distro)
- **Flutter version** (`flutter --version`)
- **Steps to reproduce** the issue
- **Expected behavior** vs **actual behavior**
- **Logs or screenshots** if applicable
- **Crash reports** if the app crashes

Use the [GitHub Issues](https://github.com/deepc0py/WarpDeck/issues) page with the "bug" label.

## 💡 Feature Requests

For new features:

- Check if the feature already exists or is planned
- Describe the **use case** and **benefit**
- Provide **mockups or examples** if applicable
- Consider **implementation complexity**

Use the [GitHub Issues](https://github.com/deepc0py/WarpDeck/issues) page with the "enhancement" label.

## 🏷️ Areas for Contribution

We welcome contributions in these areas:

### 🐛 Bug Fixes
- Crash fixes and stability improvements
- Performance optimizations
- Cross-platform compatibility issues

### ✨ Features
- New file transfer features
- UI/UX improvements
- Security enhancements
- Platform-specific integrations

### 📖 Documentation
- Code documentation
- User guides and tutorials
- API documentation
- Translation and localization

### 🧪 Testing
- Unit tests for core functionality
- Integration tests
- Platform-specific testing
- Performance benchmarks

### 🎨 Design
- UI/UX improvements
- Icons and graphics
- Accessibility enhancements
- Material Design compliance

## 🔒 Security

If you discover a security vulnerability:

1. **DO NOT** open a public issue
2. **Email us directly** at security@warpdeck.dev
3. **Include** detailed steps to reproduce
4. **Wait** for our response before public disclosure

We take security seriously and will respond promptly to legitimate security concerns.

## 📄 License

By contributing to WarpDeck, you agree that your contributions will be licensed under the [MIT License](LICENSE).

## 💬 Community

- **GitHub Discussions**: [Ask questions and share ideas](https://github.com/deepc0py/WarpDeck/discussions)
- **Issues**: [Report bugs and request features](https://github.com/deepc0py/WarpDeck/issues)
- **Discord**: [Join our community](https://discord.gg/warpdeck) *(coming soon)*

## 🙏 Recognition

Contributors will be recognized in:
- The project README
- Release notes for significant contributions
- Our website's contributors page *(coming soon)*

---

**Thank you for helping make WarpDeck better!** 🎉