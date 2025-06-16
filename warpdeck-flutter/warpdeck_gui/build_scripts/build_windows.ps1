# WarpDeck Windows Build Script (PowerShell)
# This script builds the WarpDeck Flutter application for Windows

Write-Host "Building WarpDeck for Windows..." -ForegroundColor Green

# Check if Flutter is installed
try {
    $flutterVersion = flutter --version 2>$null
    Write-Host "Flutter found: $($flutterVersion.Split([Environment]::NewLine)[0])" -ForegroundColor Cyan
} catch {
    Write-Host "Error: Flutter is not installed or not in PATH" -ForegroundColor Red
    Write-Host "Please install Flutter and add it to your PATH" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if Visual Studio Build Tools are available
try {
    $null = Get-Command cl.exe -ErrorAction Stop
    Write-Host "Visual Studio Build Tools found" -ForegroundColor Cyan
} catch {
    Write-Host "Error: Visual Studio Build Tools not found" -ForegroundColor Red
    Write-Host "Please install Visual Studio 2019 or later with C++ build tools" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Clean previous builds
Write-Host "Cleaning previous builds..." -ForegroundColor Yellow
if (Test-Path "build\windows") {
    Remove-Item -Recurse -Force "build\windows"
}

# Get dependencies
Write-Host "Getting Flutter dependencies..." -ForegroundColor Yellow
$result = flutter pub get
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Failed to get Flutter dependencies" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Generate FFI bindings if needed
Write-Host "Generating FFI bindings..." -ForegroundColor Yellow
$result = dart run build_runner build --delete-conflicting-outputs
if ($LASTEXITCODE -ne 0) {
    Write-Host "Warning: FFI binding generation failed, continuing anyway..." -ForegroundColor Yellow
}

# Build for Windows
Write-Host "Building Windows application..." -ForegroundColor Yellow
$result = flutter build windows --release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Windows build failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "====================================" -ForegroundColor Green
Write-Host "Windows build completed successfully!" -ForegroundColor Green
Write-Host "====================================" -ForegroundColor Green
Write-Host ""
Write-Host "Executable location: build\windows\x64\runner\Release\warpdeck_gui.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "You can now run the application or create an installer." -ForegroundColor Yellow
Write-Host ""
Read-Host "Press Enter to exit"