#!/bin/bash

# WarpDeck VM Testing Setup Script for VMware Fusion on Apple Silicon
# Creates and configures a Debian ARM64 VM for mDNS testing

set -e

VMRUN="/Applications/VMware Fusion.app/Contents/Public/vmrun"
VMWARE_VDISKMANAGER="/Applications/VMware Fusion.app/Contents/Library/vmware-vdiskmanager"
ISO_PATH="/Users/jesse/Downloads/debian-13.0.0-arm64-netinst-2.iso"
VM_NAME="WarpDeck-Debian-Test"
VM_PATH="$HOME/Virtual Machines.localized/${VM_NAME}.vmwarevm"
VMX_FILE="${VM_PATH}/${VM_NAME}.vmx"
VMDK_FILE="${VM_PATH}/${VM_NAME}.vmdk"

echo "🚀 Setting up Debian ARM64 VM for WarpDeck testing..."

# Check if VM already exists
if [ -d "$VM_PATH" ]; then
    echo "⚠️  VM already exists at $VM_PATH"
    read -p "Delete existing VM? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # Stop VM if running
        "$VMRUN" stop "$VMX_FILE" soft 2>/dev/null || true
        rm -rf "$VM_PATH"
        echo "✅ Removed existing VM"
    else
        echo "Using existing VM"
        exit 0
    fi
fi

# Create VM directory
echo "📁 Creating VM directory..."
mkdir -p "$VM_PATH"

# Create virtual disk (10GB)
echo "💾 Creating 10GB virtual disk..."
"$VMWARE_VDISKMANAGER" -c -s 10GB -t 0 "$VMDK_FILE"

# Create VMX configuration file for ARM64
echo "⚙️  Creating ARM64 VM configuration..."
cat > "$VMX_FILE" << 'EOF'
.encoding = "UTF-8"
config.version = "8"
virtualHW.version = "20"
displayName = "WarpDeck-Debian-Test"
guestOS = "arm-debian-64"
memsize = "2048"
numvcpus = "2"
firmware = "efi"

# Storage
scsi0.present = "TRUE"
scsi0.virtualDev = "virtio-scsi"
scsi0:0.present = "TRUE"
scsi0:0.fileName = "WarpDeck-Debian-Test.vmdk"

# CD-ROM with ISO
sata0.present = "TRUE"
sata0:0.present = "TRUE"
sata0:0.deviceType = "cdrom-image"
sata0:0.fileName = "/Users/jesse/Downloads/debian-13.0.0-arm64-netinst-2.iso"

# Networking - CRITICAL: Bridged for mDNS
ethernet0.present = "TRUE"
ethernet0.connectionType = "bridged"
ethernet0.virtualDev = "vmxnet3"
ethernet0.addressType = "generated"
ethernet0.linkStatePropagation.enable = "TRUE"

# Basic hardware
usb.present = "TRUE"
ehci.present = "TRUE"
usb_xhci.present = "TRUE"
sound.present = "FALSE"
vmci0.present = "TRUE"
hpet0.present = "TRUE"
tools.syncTime = "FALSE"

# Power management
powerType.powerOff = "soft"
powerType.powerOn = "soft"
powerType.suspend = "soft"
powerType.reset = "soft"

# ARM specific
vhv.enable = "TRUE"
vpmc.enable = "FALSE"

# Tools
tools.upgrade.policy = "useGlobal"
EOF

echo "✅ VM configuration created for ARM64"

# Verify ISO exists
if [ ! -f "$ISO_PATH" ]; then
    echo "❌ ISO file not found at: $ISO_PATH"
    exit 1
fi

# Start the VM
echo "🎬 Starting VM with Debian installer..."
echo "   VMware Fusion will open with the installer"
"$VMRUN" start "$VMX_FILE" gui 2>&1 || {
    echo "⚠️  If VM fails to start, try opening VMware Fusion manually and:"
    echo "   1. Create new VM from: $ISO_PATH"
    echo "   2. Choose 'Debian 12.x 64-bit Arm'"
    echo "   3. Set network to BRIDGED mode"
    echo "   4. Name it: WarpDeck-Debian-Test"
}

echo ""
echo "📝 Installation Instructions:"
echo "1. Complete Debian installation"
echo "2. Choose minimal install (no desktop environment)"
echo "3. Set hostname to: debian-warpdeck"
echo "4. Create user: warpdeck"
echo "5. Enable SSH server"
echo ""
echo "📋 After installation, run in the VM:"
echo ""
cat << 'SETUP_SCRIPT'
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake git avahi-daemon avahi-utils

# Test mDNS is working
sudo systemctl start avahi-daemon
avahi-browse -a

# Get the fixed WarpDeck code
git clone https://github.com/your-repo/WarpDeck.git
cd WarpDeck/libwarpdeck
mkdir build && cd build
cmake ..
make -j2
SETUP_SCRIPT

echo ""
echo "🔍 VM Control Commands:"
echo "   Stop:  $VMRUN stop \"$VMX_FILE\" soft"
echo "   Start: $VMRUN start \"$VMX_FILE\" gui"
echo "   IP:    $VMRUN getGuestIPAddress \"$VMX_FILE\""