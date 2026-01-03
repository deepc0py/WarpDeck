#!/bin/bash

# UTM Debian Setup Guide for WarpDeck Testing
# UTM is much better for ARM64 VMs on Apple Silicon

ISO_PATH="/Users/jesse/Downloads/debian-13.0.0-arm64-netinst-2.iso"

echo "🚀 UTM Debian VM Setup for WarpDeck Testing"
echo "==========================================="
echo ""
echo "UTM will handle ARM64 VMs much better than VirtualBox!"
echo ""
echo "📋 MANUAL SETUP STEPS IN UTM:"
echo ""
echo "1. Open UTM (should be opening now...)"
open -a UTM

echo ""
echo "2. Click 'Create a New Virtual Machine'"
echo ""
echo "3. Select 'Virtualize' (not Emulate)"
echo ""
echo "4. Choose 'Linux'"
echo ""
echo "5. Use these settings:"
echo "   • Boot ISO: Browse to $ISO_PATH"
echo "   • Memory: 2048 MB"
echo "   • CPU Cores: 2"
echo "   • Storage: 10 GB"
echo "   • Name: WarpDeck-Debian-Test"
echo ""
echo "6. IMPORTANT - Network Settings:"
echo "   • Click 'Network' in the sidebar"
echo "   • Change 'Network Mode' from 'Shared' to 'Bridged'"
echo "   • Select your Wi-Fi adapter (en0)"
echo "   • This is CRITICAL for mDNS to work!"
echo ""
echo "7. Save and Start the VM"
echo ""
echo "==============================================="
echo "📝 DEBIAN INSTALLATION:"
echo "==============================================="
echo "• Choose 'Install' (text mode)"
echo "• Language: English"
echo "• Hostname: debian-warpdeck"
echo "• Username: warpdeck"
echo "• Partitioning: Guided - use entire disk"
echo "• Software: SSH server + standard utilities (no desktop)"
echo ""
echo "==============================================="
echo "🔧 AFTER INSTALLATION:"
echo "==============================================="
cat << 'SCRIPT'
# In the VM:
ip addr show  # Note the IP address

# Install dependencies:
sudo apt update
sudo apt install -y build-essential cmake git avahi-daemon avahi-utils

# Start mDNS:
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon

# Test mDNS:
avahi-browse -a

# From Mac, transfer the fixed code:
# scp -r /Users/jesse/code/WarpDeck warpdeck@<VM-IP>:~/

# Build WarpDeck:
cd ~/WarpDeck/libwarpdeck
mkdir build && cd build
cmake ..
make -j2

# Run test:
./test_mdns_cross_platform --announce --id debian-vm
SCRIPT

echo ""
echo "==============================================="
echo "🎯 TESTING mDNS BETWEEN MAC AND VM:"
echo "==============================================="
echo ""
echo "On Mac:"
echo "  cd /Users/jesse/code/WarpDeck"
echo "  ./test_mdns_cross_platform"
echo ""
echo "On Debian VM:"
echo "  ./test_mdns_cross_platform --announce --id debian-vm"
echo ""
echo "Both should discover each other with the fix applied!"
echo ""
echo "The key fix: We changed from ephemeral ports to proper"
echo "mDNS port 5353 binding with graceful fallback."