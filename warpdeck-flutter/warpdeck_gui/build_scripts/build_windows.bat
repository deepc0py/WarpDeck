@echo off
REM WarpDeck Windows Build Script
REM This script builds the WarpDeck Flutter application for Windows

echo Building WarpDeck for Windows...

REM Check if Flutter is installed
flutter --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Flutter is not installed or not in PATH
    echo Please install Flutter and add it to your PATH
    pause
    exit /b 1
)

REM Check if Visual Studio Build Tools are available
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Visual Studio Build Tools not found
    echo Please install Visual Studio 2019 or later with C++ build tools
    pause
    exit /b 1
)

REM Clean previous builds
echo Cleaning previous builds...
if exist "build\windows" rmdir /s /q "build\windows"

REM Get dependencies
echo Getting Flutter dependencies...
flutter pub get
if %errorlevel% neq 0 (
    echo Error: Failed to get Flutter dependencies
    pause
    exit /b 1
)

REM Generate FFI bindings if needed
echo Generating FFI bindings...
dart run build_runner build --delete-conflicting-outputs
if %errorlevel% neq 0 (
    echo Warning: FFI binding generation failed, continuing anyway...
)

REM Build for Windows
echo Building Windows application...
flutter build windows --release
if %errorlevel% neq 0 (
    echo Error: Windows build failed
    pause
    exit /b 1
)

echo.
echo ====================================
echo Windows build completed successfully!
echo ====================================
echo.
echo Executable location: build\windows\x64\runner\Release\warpdeck_gui.exe
echo.
echo You can now run the application or create an installer.
echo.
pause