# VM Testing Guide for WarpDeck mDNS Fix

## VM Setup Instructions

### 1. Create Linux VM (Ubuntu recommended)
- **RAM**: 2GB minimum
- **Disk**: 10GB
- **Network**: **BRIDGED MODE** (Critical!)
  - This ensures VM gets IP on same subnet as host
  - Enables proper multicast routing

### 2. Install Dependencies in VM
```bash
sudo apt update
sudo apt install -y build-essential cmake git
sudo apt install -y avahi-utils  # For mDNS testing tools

# Clone WarpDeck
git clone <your-repo-url>
cd WarpDeck
```

### 3. Build on Linux
```bash
cd libwarpdeck
mkdir build && cd build
cmake ..
make -j4
```

### 4. Network Configuration Checks

#### On Both Mac and Linux:
```bash
# Check multicast group membership
netstat -g | grep 224.0.0.251

# Check if mDNS port is available
sudo lsof -i :5353

# Test basic mDNS discovery (Linux)
avahi-browse -a

# Test basic mDNS discovery (Mac)
dns-sd -B _services._dns-sd._udp
```

### 5. Firewall Configuration

#### Linux VM:
```bash
# Check firewall status
sudo ufw status

# If enabled, allow mDNS
sudo ufw allow 5353/udp

# Or temporarily disable for testing
sudo ufw disable
```

#### Mac Host:
System Preferences → Security & Privacy → Firewall → Turn Off (temporarily)

### 6. Test mDNS Communication

#### Quick Test with Native Tools:

**On Linux VM:**
```bash
# Register a test service
avahi-publish-service test-linux _warpdeck._tcp 54321 "test=linux"
```

**On Mac Host:**
```bash
# Should see the Linux service
dns-sd -B _warpdeck._tcp
```

### 7. Test WarpDeck

**Terminal 1 (Mac Host):**
```bash
cd /Users/jesse/code/WarpDeck
./warpdeck-flutter/warpdeck_gui/build/macos/Build/Products/Debug/warpdeck_gui.app/Contents/MacOS/warpdeck_gui
```

**Terminal 2 (Linux VM):**
```bash
cd ~/WarpDeck
./build/warpdeck_test  # Or your test binary
```

### 8. Debugging Tips

1. **Use Wireshark** to capture mDNS traffic:
   ```bash
   # Filter: udp.port == 5353
   ```

2. **Check multicast routes:**
   ```bash
   # Linux
   ip route show | grep multicast
   
   # Mac
   netstat -nr | grep 224
   ```

3. **Test with sudo** if binding to port 5353 fails:
   ```bash
   sudo ./your_test_binary
   ```

4. **Check VM network adapter**:
   - Ensure it's in bridged mode
   - VM should have IP like 192.168.x.x (same subnet as host)

### Expected Results

After applying the fix:
1. Devices should discover each other within 3-5 seconds
2. You'll see log messages showing successful socket binding
3. Each device appears in the other's peer list

### Common Issues

1. **"Cannot bind to mDNS port 5353"**
   - Normal for non-root; falls back to ephemeral port
   - Still works but may be less reliable

2. **No discovery between devices**
   - Check VM is using bridged networking
   - Verify multicast is enabled on network interfaces
   - Temporarily disable all firewalls

3. **Intermittent discovery**
   - This was the bug we fixed! Should be resolved now

## What We Fixed

The bug was in `mdns_manager.cpp`:
- **Before**: Used ephemeral ports, couldn't join multicast group properly
- **After**: Attempts to bind to port 5353, falls back gracefully, proper multicast membership

This enables Mac ↔ Linux discovery to work reliably!