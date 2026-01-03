#!/bin/bash

# WarpDeck VM Testing Setup Script for VirtualBox
# Creates and configures a Debian ARM64 VM for mDNS testing

set -e

VBOX="VBoxManage"
ISO_PATH="/Users/jesse/Downloads/debian-13.0.0-arm64-netinst-2.iso"
VM_NAME="WarpDeck-Debian-Test"
VM_PATH="$HOME/VirtualBox VMs/${VM_NAME}"

echo "🚀 Setting up Debian ARM64 VM in VirtualBox for WarpDeck testing..."

# Check if VM already exists
if $VBOX list vms | grep -q "\"$VM_NAME\""; then
    echo "⚠️  VM '$VM_NAME' already exists"
    read -p "Delete existing VM? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing existing VM..."
        $VBOX unregistervm "$VM_NAME" --delete 2>/dev/null || true
        echo "✅ Removed existing VM"
    else
        echo "Using existing VM"
        exit 0
    fi
fi

# Check ISO exists
if [ ! -f "$ISO_PATH" ]; then
    echo "❌ ISO not found at: $ISO_PATH"
    exit 1
fi

echo "📦 Creating new ARM64 VM..."
# Create VM with ARM64 architecture
$VBOX createvm --name "$VM_NAME" --ostype "Debian_arm64" --register

echo "⚙️  Configuring VM settings..."
# Configure VM settings
$VBOX modifyvm "$VM_NAME" \
    --memory 2048 \
    --cpus 2 \
    --firmware efi \
    --boot1 dvd \
    --boot2 disk \
    --boot3 none \
    --boot4 none \
    --audio-driver none \
    --clipboard bidirectional

echo "💾 Creating virtual disk (10GB)..."
# Create and attach storage
$VBOX createhd --filename "$VM_PATH/${VM_NAME}.vdi" --size 10240 --format VDI
$VBOX storagectl "$VM_NAME" --name "SATA Controller" --add sata --controller IntelAhci
$VBOX storageattach "$VM_NAME" --storagectl "SATA Controller" --port 0 --device 0 \
    --type hdd --medium "$VM_PATH/${VM_NAME}.vdi"

echo "💿 Attaching Debian ISO..."
# Attach ISO
$VBOX storageattach "$VM_NAME" --storagectl "SATA Controller" --port 1 --device 0 \
    --type dvddrive --medium "$ISO_PATH"

echo "🌐 Configuring network for mDNS (Bridged mode)..."
# CRITICAL: Configure bridged networking for mDNS
# Use the Wi-Fi adapter for bridged networking
BRIDGE_ADAPTER="en0: Wi-Fi"

$VBOX modifyvm "$VM_NAME" \
    --nic1 bridged \
    --bridgeadapter1 "$BRIDGE_ADAPTER" \
    --nictype1 virtio

echo "   Using bridged adapter: $BRIDGE_ADAPTER"

echo "✅ VM created successfully!"
echo ""
echo "🎬 Starting VM with Debian installer..."
$VBOX startvm "$VM_NAME"

echo ""
echo "==============================================="
echo "📝 DEBIAN INSTALLATION INSTRUCTIONS:"
echo "==============================================="
echo "1. Select 'Install' (not graphical)"
echo "2. Language: English"
echo "3. Location: United States"
echo "4. Keyboard: American English"
echo "5. Hostname: debian-warpdeck"
echo "6. Domain: (leave empty)"
echo "7. Root password: (set a secure password)"
echo "8. User: warpdeck"
echo "9. Partition: Guided - use entire disk"
echo "10. Software selection:"
echo "    - UNCHECK 'Debian desktop environment'"
echo "    - CHECK 'SSH server'"
echo "    - CHECK 'standard system utilities'"
echo ""
echo "==============================================="
echo "📋 AFTER INSTALLATION, RUN IN VM:"
echo "==============================================="
cat << 'SETUP_SCRIPT'
# Login as warpdeck user, then:

# Install build tools
sudo apt update
sudo apt install -y build-essential cmake git

# Install mDNS tools
sudo apt install -y avahi-daemon avahi-utils

# Start Avahi daemon
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon

# Test mDNS
avahi-browse -a

# Get IP address
ip addr show

# Clone and build WarpDeck (you'll need to transfer the fixed code)
# Option 1: Via git (if you push the fixes)
# git clone https://github.com/your-repo/WarpDeck.git

# Option 2: Via SCP from Mac host
# On Mac: scp -r /Users/jesse/code/WarpDeck warpdeck@<VM-IP>:~/

# Build WarpDeck
cd ~/WarpDeck/libwarpdeck
mkdir build && cd build
cmake ..
make -j2

# Test discovery
./warpdeck_test  # or your test binary
SETUP_SCRIPT

echo ""
echo "==============================================="
echo "🔧 VM CONTROL COMMANDS:"
echo "==============================================="
echo "Stop VM:    $VBOX controlvm \"$VM_NAME\" poweroff"
echo "Start VM:   $VBOX startvm \"$VM_NAME\""
echo "Headless:   $VBOX startvm \"$VM_NAME\" --type headless"
echo "VM Info:    $VBOX showvminfo \"$VM_NAME\""
echo "Delete VM:  $VBOX unregistervm \"$VM_NAME\" --delete"
echo ""
echo "📡 NETWORK INFO:"
echo "The VM is configured with BRIDGED networking on: $DEFAULT_INTERFACE"
echo "This is required for mDNS multicast to work between host and VM!"
echo ""
echo "🔍 To get VM IP after installation:"
echo "   1. Login to VM console"
echo "   2. Run: ip addr show"
echo "   3. Or from Mac (if VM tools installed): $VBOX guestproperty get \"$VM_NAME\" \"/VirtualBox/GuestInfo/Net/0/V4/IP\""