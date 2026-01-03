#!/bin/bash

# WarpDeck VM Testing Setup Script for VMware Fusion
# Creates and configures a Debian VM for mDNS testing

set -e

VMRUN="/Applications/VMware Fusion.app/Contents/Public/vmrun"
VMWARE_VDISKMANAGER="/Applications/VMware Fusion.app/Contents/Library/vmware-vdiskmanager"
ISO_PATH="/Users/jesse/Downloads/debian-13.0.0-arm64-netinst-2.iso"
VM_NAME="WarpDeck-Debian-Test"
VM_PATH="$HOME/Virtual Machines.localized/${VM_NAME}.vmwarevm"
VMX_FILE="${VM_PATH}/${VM_NAME}.vmx"
VMDK_FILE="${VM_PATH}/${VM_NAME}.vmdk"

echo "🚀 Setting up Debian VM for WarpDeck testing..."

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
"$VMWARE_VDISKMANAGER" -c -s 10GB -a lsilogic -t 0 "$VMDK_FILE"

# Create VMX configuration file
echo "⚙️  Creating VM configuration..."
cat > "$VMX_FILE" << 'EOF'
.encoding = "UTF-8"
config.version = "8"
virtualHW.version = "20"
displayName = "WarpDeck-Debian-Test"
guestOS = "arm-debian12-64"
memsize = "2048"
numvcpus = "2"
cpuid.coresPerSocket = "1"
scsi0.present = "TRUE"
scsi0.virtualDev = "lsilogic"
scsi0:0.present = "TRUE"
scsi0:0.fileName = "WarpDeck-Debian-Test.vmdk"
ethernet0.present = "TRUE"
ethernet0.connectionType = "bridged"
ethernet0.virtualDev = "vmxnet3"
ethernet0.wakeOnPcktRcv = "FALSE"
ethernet0.addressType = "generated"
ethernet0.linkStatePropagation.enable = "TRUE"
usb.present = "TRUE"
sound.present = "TRUE"
sound.virtualDev = "hdaudio"
sound.fileName = "-1"
sound.autodetect = "TRUE"
pciBridge0.present = "TRUE"
pciBridge4.present = "TRUE"
pciBridge4.virtualDev = "pcieRootPort"
pciBridge4.functions = "8"
pciBridge5.present = "TRUE"
pciBridge5.virtualDev = "pcieRootPort"
pciBridge5.functions = "8"
pciBridge6.present = "TRUE"
pciBridge6.virtualDev = "pcieRootPort"
pciBridge6.functions = "8"
pciBridge7.present = "TRUE"
pciBridge7.virtualDev = "pcieRootPort"
pciBridge7.functions = "8"
vmci0.present = "TRUE"
hpet0.present = "TRUE"
tools.syncTime = "FALSE"
powerType.powerOff = "soft"
powerType.powerOn = "soft"
powerType.suspend = "soft"
powerType.reset = "soft"
sata0.present = "TRUE"
sata0:1.present = "TRUE"
sata0:1.deviceType = "cdrom-image"
sata0:1.fileName = "/Users/jesse/Downloads/debian-13.0.0-arm64-netinst-2.iso"
sata0:1.autodetect = "TRUE"
virtualHW.productCompatibility = "hosted"
vhv.enable = "TRUE"
vpmc.enable = "TRUE"
vvtd.enable = "TRUE"
tools.upgrade.policy = "useGlobal"
ehci.present = "TRUE"
ehci.pciSlotNumber = "0"
firmware = "efi"

# Important for mDNS - bridged networking
ethernet0.connectionType = "bridged"
# Auto-detect the best physical adapter
ethernet0.autoDetect = "TRUE"
EOF

echo "✅ VM configuration created"

# Start the VM
echo "🎬 Starting VM with Debian installer..."
echo "   Note: You'll need to complete the Debian installation manually"
"$VMRUN" start "$VMX_FILE" gui

echo ""
echo "📝 Installation Instructions:"
echo "1. Complete Debian installation (basic install, no GUI needed)"
echo "2. Set hostname to: debian-warpdeck"
echo "3. Create user: warpdeck"
echo "4. Enable SSH server during installation"
echo ""
echo "📋 After installation, run this in the VM:"
echo ""
cat << 'SETUP_SCRIPT'
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake git avahi-daemon avahi-utils

# Configure avahi for better mDNS
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon

# Clone and build WarpDeck
git clone https://github.com/your-repo/WarpDeck.git
cd WarpDeck/libwarpdeck
mkdir build && cd build
cmake ..
make -j2

# Test mDNS locally
avahi-browse -a
SETUP_SCRIPT

echo ""
echo "🔍 To check VM network configuration:"
echo "   $VMRUN getGuestIPAddress \"$VMX_FILE\""
echo ""
echo "🛑 To stop the VM:"
echo "   $VMRUN stop \"$VMX_FILE\" soft"
echo ""
echo "▶️  To start the VM later:"
echo "   $VMRUN start \"$VMX_FILE\" gui"
echo ""
echo "📡 IMPORTANT: The VM is configured with BRIDGED networking for mDNS!"