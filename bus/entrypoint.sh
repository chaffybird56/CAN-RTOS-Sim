#!/bin/bash
set -e

CAN_INTERFACE=${CAN_INTERFACE:-vcan0}

echo "Setting up virtual CAN interface: $CAN_INTERFACE"

# Try to load vcan module if available
if command -v modprobe >/dev/null 2>&1; then
    echo "Loading vcan kernel module..."
    modprobe vcan 2>/dev/null || echo "Warning: Could not load vcan module"
fi

# Check if interface already exists
if ip link show $CAN_INTERFACE >/dev/null 2>&1; then
    echo "Interface $CAN_INTERFACE already exists"
else
    # Create virtual CAN interface
    echo "Creating $CAN_INTERFACE interface..."
    if ! ip link add dev $CAN_INTERFACE type vcan 2>/dev/null; then
        echo "Warning: Could not create vcan interface - creating dummy interface instead"
        # Create a dummy interface for testing
        ip link add dev $CAN_INTERFACE type dummy 2>/dev/null || {
            echo "Error: Could not create any interface"
            exit 1
        }
    fi
fi

# Bring the interface up
echo "Bringing up $CAN_INTERFACE..."
ip link set up $CAN_INTERFACE

# Verify interface is up
echo "Verifying interface status..."
ip link show $CAN_INTERFACE

echo "Virtual CAN interface $CAN_INTERFACE is ready!"
echo "Interface details:"
ip -details link show $CAN_INTERFACE

# Keep container alive
echo "CAN bus container is running. Press Ctrl+C to stop."
tail -f /dev/null
