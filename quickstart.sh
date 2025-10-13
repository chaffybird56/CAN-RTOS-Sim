#!/bin/bash

# CAN-RTOS-Sim Quick Start Script
# One-command setup and launch

set -e

echo "🚀 CAN-RTOS-Sim Quick Start"
echo "=========================="
echo

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    local status=$1
    local message=$2
    case $status in
        "OK")
            echo -e "${GREEN}✓${NC} $message"
            ;;
        "INFO")
            echo -e "${BLUE}ℹ${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}⚠${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}✗${NC} $message"
            ;;
    esac
}

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v docker &> /dev/null; then
    print_status "ERROR" "Docker is not installed"
    echo "Please install Docker Desktop for Mac: https://www.docker.com/products/docker-desktop/"
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    print_status "ERROR" "Docker is not running"
    echo "Please start Docker Desktop"
    exit 1
fi

print_status "OK" "Docker is ready"

# Build and start services
echo
echo "Building and starting services..."
print_status "INFO" "This may take a few minutes on first run"

# Build images
print_status "INFO" "Building Docker images..."
docker compose -f ops/compose.yml build

# Start bus first
print_status "INFO" "Starting virtual CAN bus..."
docker compose -f ops/compose.yml up -d bus

# Wait for bus to be ready
sleep 3

# Start other services
print_status "INFO" "Starting nodes and dashboard..."
docker compose -f ops/compose.yml up -d node_a node_b server panel

# Wait for services to be ready
print_status "INFO" "Waiting for services to initialize..."
sleep 10

# Check if services are running
echo
echo "Checking service status..."
services=("bus" "node_a" "node_b" "server" "panel")
all_ready=true

for service in "${services[@]}"; do
    if docker compose -f ops/compose.yml ps --services --filter "status=running" | grep -q "^${service}$"; then
        print_status "OK" "$service is running"
    else
        print_status "ERROR" "$service is not running"
        all_ready=false
    fi
done

if [ "$all_ready" = false ]; then
    echo
    print_status "WARN" "Some services failed to start"
    echo "Check logs with: docker compose -f ops/compose.yml logs"
    exit 1
fi

# Check CAN interface
echo
print_status "INFO" "Verifying CAN interface..."
if docker compose -f ops/compose.yml exec -T bus ip link show vcan0 >/dev/null 2>&1; then
    print_status "OK" "vcan0 interface is ready"
else
    print_status "ERROR" "vcan0 interface not found"
    exit 1
fi

# Check API
print_status "INFO" "Testing API..."
if curl -s http://localhost:8000/health >/dev/null 2>&1; then
    print_status "OK" "Server API is responding"
else
    print_status "WARN" "Server API not responding yet"
fi

# Check dashboard
print_status "INFO" "Testing dashboard..."
if curl -s http://localhost:3000 >/dev/null 2>&1; then
    print_status "OK" "Dashboard is accessible"
else
    print_status "WARN" "Dashboard not ready yet"
fi

echo
echo "🎉 CAN-RTOS-Sim is ready!"
echo
echo "📊 Dashboard: http://localhost:3000"
echo "🔧 API: http://localhost:8000"
echo
echo "📋 Quick Commands:"
echo "  View logs:     docker compose -f ops/compose.yml logs -f"
echo "  Monitor CAN:   docker compose -f ops/compose.yml exec bus candump -l vcan0"
echo "  Stop system:   docker compose -f ops/compose.yml down"
echo
echo "🎯 Try the demo:"
echo "  1. Open http://localhost:3000"
echo "  2. Click 'Drop Sensor' in Fault Injection panel"
echo "  3. Watch Node B state change to DEGRADED"
echo "  4. Wait for automatic recovery"
echo
echo "📖 For more commands, see ops/Makefile"

# Open dashboard if possible
if command -v open &> /dev/null; then
    print_status "INFO" "Opening dashboard in browser..."
    open http://localhost:3000
fi

