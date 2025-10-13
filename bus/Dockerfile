FROM ubuntu:22.04

# Install CAN utilities and network tools
RUN apt-get update && apt-get install -y \
    can-utils \
    iproute2 \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Copy entrypoint script
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Health check
HEALTHCHECK --interval=5s --timeout=3s --retries=3 \
    CMD ip link show vcan0 > /dev/null || exit 1

ENTRYPOINT ["/entrypoint.sh"]

