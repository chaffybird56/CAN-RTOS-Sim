#!/bin/bash

# CAN-RTOS-Sim Validation Script
# Quick sanity checks for the virtual CAN bus and nodes

set -e

echo "=== CAN-RTOS-Sim Validation Script ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "OK")
            echo -e "${GREEN}✓${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}⚠${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}✗${NC} $message"
            ;;
    esac
}

# Check if Docker is running
echo "1. Checking Docker status..."
if docker info >/dev/null 2>&1; then
    print_status "OK" "Docker is running"
else
    print_status "ERROR" "Docker is not running"
    exit 1
fi

# Check if compose file exists
echo "2. Checking compose configuration..."
if [ -f "ops/compose.yml" ]; then
    print_status "OK" "Docker Compose file found"
else
    print_status "ERROR" "Docker Compose file not found"
    exit 1
fi

# Check if services are running
echo "3. Checking service status..."
services=("bus" "node_a" "node_b" "server" "panel")
for service in "${services[@]}"; do
    if docker compose -f ops/compose.yml ps --services --filter "status=running" | grep -q "^${service}$"; then
        print_status "OK" "Service $service is running"
    else
        print_status "WARN" "Service $service is not running"
    fi
done

# Check if vcan0 interface exists
echo "4. Checking virtual CAN interface..."
if docker compose -f ops/compose.yml exec -T bus ip link show vcan0 >/dev/null 2>&1; then
    print_status "OK" "vcan0 interface exists"
    
    # Check if interface is up
    if docker compose -f ops/compose.yml exec -T bus ip link show vcan0 | grep -q "UP"; then
        print_status "OK" "vcan0 interface is UP"
    else
        print_status "WARN" "vcan0 interface is DOWN"
    fi
else
    print_status "ERROR" "vcan0 interface does not exist"
fi

# Check for CAN traffic
echo "5. Checking CAN traffic..."
frame_count=$(docker compose -f ops/compose.yml exec -T bus timeout 5 candump -l vcan0 2>/dev/null | wc -l || echo "0")
if [ "$frame_count" -gt 0 ]; then
    print_status "OK" "CAN traffic detected ($frame_count frames in 5 seconds)"
else
    print_status "WARN" "No CAN traffic detected"
fi

# Check server API
echo "6. Checking server API..."
if curl -s http://localhost:8000/health >/dev/null 2>&1; then
    print_status "OK" "Server API is responding"
    
    # Get metrics
    if curl -s http://localhost:8000/metrics >/dev/null 2>&1; then
        print_status "OK" "Metrics endpoint is working"
    else
        print_status "WARN" "Metrics endpoint not responding"
    fi
else
    print_status "WARN" "Server API is not responding"
fi

# Check dashboard
echo "7. Checking dashboard..."
if curl -s http://localhost:3000 >/dev/null 2>&1; then
    print_status "OK" "Dashboard is accessible"
else
    print_status "WARN" "Dashboard is not accessible"
fi

echo
echo "=== Validation Complete ==="
echo
echo "Next steps:"
echo "- If all checks passed, your system is working correctly"
echo "- If warnings appeared, the system may still function but with reduced capabilities"
echo "- If errors occurred, check the service logs: make logs"
echo
echo "Useful commands:"
echo "- Monitor CAN traffic: make can-monitor"
echo "- View logs: make logs"
echo "- Open dashboard: make dashboard"
echo "- Inject faults: make inject-drop, make inject-flood, etc."

