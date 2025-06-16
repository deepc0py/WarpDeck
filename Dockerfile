FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libssl-dev \
    libavahi-client-dev \
    libavahi-common-dev \
    avahi-daemon \
    avahi-utils \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Copy source code
COPY . .

# Build libwarpdeck
WORKDIR /app/libwarpdeck
RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Create test directory with some files to transfer
RUN mkdir -p /app/test_files && \
    echo "Hello from Docker container!" > /app/test_files/hello.txt && \
    echo "This is a test file for WarpDeck transfer" > /app/test_files/test.txt && \
    date > /app/test_files/timestamp.txt

# Set up Avahi daemon configuration
RUN mkdir -p /etc/avahi && \
    echo "[server]" > /etc/avahi/avahi-daemon.conf && \
    echo "host-name=Linux-Docker-WarpDeck" >> /etc/avahi/avahi-daemon.conf && \
    echo "domain-name=local" >> /etc/avahi/avahi-daemon.conf && \
    echo "browse-domains=0pointer.de, zeroconf.org" >> /etc/avahi/avahi-daemon.conf && \
    echo "use-ipv4=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "use-ipv6=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "allow-interfaces=eth0" >> /etc/avahi/avahi-daemon.conf && \
    echo "check-response-ttl=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "use-iff-running=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "enable-dbus=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "disallow-other-stacks=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "allow-point-to-point=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "cache-entries-max=4096" >> /etc/avahi/avahi-daemon.conf && \
    echo "clients-max=4096" >> /etc/avahi/avahi-daemon.conf && \
    echo "objects-per-client-max=1024" >> /etc/avahi/avahi-daemon.conf && \
    echo "entries-per-entry-group-max=32" >> /etc/avahi/avahi-daemon.conf && \
    echo "ratelimit-interval-usec=1000000" >> /etc/avahi/avahi-daemon.conf && \
    echo "ratelimit-burst=1000" >> /etc/avahi/avahi-daemon.conf && \
    echo "" >> /etc/avahi/avahi-daemon.conf && \
    echo "[wide-area]" >> /etc/avahi/avahi-daemon.conf && \
    echo "enable-wide-area=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "" >> /etc/avahi/avahi-daemon.conf && \
    echo "[publish]" >> /etc/avahi/avahi-daemon.conf && \
    echo "disable-publishing=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "disable-user-service-publishing=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "add-service-cookie=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-addresses=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-hinfo=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-workstation=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-domain=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-dns-servers=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-resolv-conf-dns-servers=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-aaaa-on-ipv4=yes" >> /etc/avahi/avahi-daemon.conf && \
    echo "publish-a-on-ipv6=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "" >> /etc/avahi/avahi-daemon.conf && \
    echo "[reflector]" >> /etc/avahi/avahi-daemon.conf && \
    echo "enable-reflector=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "reflect-ipv=no" >> /etc/avahi/avahi-daemon.conf && \
    echo "" >> /etc/avahi/avahi-daemon.conf && \
    echo "[rlimits]" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-as=" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-core=0" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-data=4194304" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-fsize=0" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-nofile=768" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-stack=4194304" >> /etc/avahi/avahi-daemon.conf && \
    echo "rlimit-nproc=3" >> /etc/avahi/avahi-daemon.conf

# Build test application
COPY test_warpdeck.cpp /app/
RUN cd /app && g++ -std=c++17 -I/app -L/app/libwarpdeck/build -o test_warpdeck test_warpdeck.cpp -lwarpdeck -pthread -lssl -lcrypto -lavahi-client -lavahi-common

# Create a simple test CLI application
RUN echo '#!/bin/bash' > /app/start_warpdeck.sh && \
    echo 'echo "Starting Avahi daemon..."' >> /app/start_warpdeck.sh && \
    echo 'service avahi-daemon start' >> /app/start_warpdeck.sh && \
    echo 'sleep 2' >> /app/start_warpdeck.sh && \
    echo 'echo "Avahi daemon status:"' >> /app/start_warpdeck.sh && \
    echo 'service avahi-daemon status' >> /app/start_warpdeck.sh && \
    echo 'echo "Browsing for WarpDeck services..."' >> /app/start_warpdeck.sh && \
    echo 'timeout 10 avahi-browse -t _warpdeck._tcp &' >> /app/start_warpdeck.sh && \
    echo 'echo "Current directory: $(pwd)"' >> /app/start_warpdeck.sh && \
    echo 'echo "Available test files:"' >> /app/start_warpdeck.sh && \
    echo 'ls -la /app/test_files/' >> /app/start_warpdeck.sh && \
    echo 'echo "Network interfaces:"' >> /app/start_warpdeck.sh && \
    echo 'ip addr show' >> /app/start_warpdeck.sh && \
    echo 'echo "Starting WarpDeck library test..."' >> /app/start_warpdeck.sh && \
    echo 'cd /app' >> /app/start_warpdeck.sh && \
    echo 'if [ -f "./test_warpdeck" ]; then' >> /app/start_warpdeck.sh && \
    echo '    echo "Running WarpDeck test application..."' >> /app/start_warpdeck.sh && \
    echo '    export LD_LIBRARY_PATH=/app/libwarpdeck/build:$LD_LIBRARY_PATH' >> /app/start_warpdeck.sh && \
    echo '    mkdir -p /run/dbus' >> /app/start_warpdeck.sh && \
    echo '    dbus-daemon --system --fork 2>/dev/null || true' >> /app/start_warpdeck.sh && \
    echo '    sleep 1' >> /app/start_warpdeck.sh && \
    echo '    ./test_warpdeck' >> /app/start_warpdeck.sh && \
    echo 'else' >> /app/start_warpdeck.sh && \
    echo '    echo "Test application not found"' >> /app/start_warpdeck.sh && \
    echo '    find /app -name "*warpdeck*" 2>/dev/null' >> /app/start_warpdeck.sh && \
    echo 'fi' >> /app/start_warpdeck.sh && \
    echo 'echo "Test completed, keeping container running for debugging..."' >> /app/start_warpdeck.sh && \
    echo 'tail -f /dev/null' >> /app/start_warpdeck.sh && \
    chmod +x /app/start_warpdeck.sh

# Expose ports for WarpDeck API server
EXPOSE 54321-65534

# Start script
CMD ["/app/start_warpdeck.sh"]