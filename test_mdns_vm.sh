#!/bin/bash

# Quick mDNS cross-platform test between Mac host and Debian VM

VMRUN="/Applications/VMware Fusion.app/Contents/Public/vmrun"
VM_NAME="WarpDeck-Debian-Test"
VMX_FILE="$HOME/Virtual Machines.localized/${VM_NAME}.vmwarevm/${VM_NAME}.vmx"

echo "🔍 WarpDeck mDNS Cross-Platform Test"
echo "====================================="

# Check if VM is running
if ! "$VMRUN" list | grep -q "$VMX_FILE"; then
    echo "⚠️  VM is not running. Starting it..."
    "$VMRUN" start "$VMX_FILE" gui
    echo "Waiting for VM to boot..."
    sleep 30
fi

# Get VM IP
echo "📡 Getting VM network info..."
VM_IP=$("$VMRUN" getGuestIPAddress "$VMX_FILE" 2>/dev/null || echo "unknown")
echo "   VM IP: $VM_IP"

# Check host network
echo "📡 Host network info:"
HOST_IP=$(ifconfig en0 | grep "inet " | awk '{print $2}')
echo "   Host IP: $HOST_IP"

# Test mDNS discovery from host
echo ""
echo "🔍 Testing mDNS discovery from Mac host..."
echo "   Looking for _warpdeck._tcp services..."

# First, try native macOS DNS-SD
timeout 5 dns-sd -B _warpdeck._tcp || true

echo ""
echo "💡 Next steps:"
echo "1. SSH into the VM: ssh warpdeck@$VM_IP"
echo "2. Run WarpDeck in the VM:"
echo "   cd ~/WarpDeck/libwarpdeck/build"
echo "   ./warpdeck_test"
echo ""
echo "3. On Mac host, run:"
echo "   cd /Users/jesse/code/WarpDeck"
echo "   ./test_mdns_cross_platform --announce --id mac-host"
echo ""
echo "4. Both devices should discover each other!"

# Quick network diagnostic
echo ""
echo "🔧 Quick Diagnostics:"
echo "-------------------"

# Check if multicast is working
echo "✓ Checking multicast routing..."
netstat -nr | grep "224\|239" | head -3

# Check if mDNS port is in use
echo ""
echo "✓ Checking mDNS port (5353)..."
sudo lsof -i :5353 2>/dev/null | head -3 || echo "   Port 5353 is available"

# Check firewall
echo ""
echo "✓ Checking firewall status..."
/usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate

echo ""
echo "📝 Troubleshooting tips:"
echo "  - Ensure VM network is in BRIDGED mode"
echo "  - Both devices should be on same subnet (${HOST_IP%.*}.x)"
echo "  - Temporarily disable firewalls for testing"
echo "  - Use 'sudo' if binding to port 5353 fails"