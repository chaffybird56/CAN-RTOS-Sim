#!/bin/bash

# CAN-RTOS-Sim Demo Recording Script
# Helps record a demonstration of the system for documentation

set -e

echo "=== CAN-RTOS-Sim Demo Recording Helper ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_step() {
    echo -e "${BLUE}Step $1:${NC} $2"
}

print_wait() {
    echo -e "${YELLOW}Waiting $1 seconds...${NC}"
    sleep $1
}

print_action() {
    echo -e "${GREEN}→${NC} $1"
}

# Check if system is ready
echo "Checking system status..."
if ! curl -s http://localhost:8000/health >/dev/null 2>&1; then
    echo -e "${RED}Error: Server is not running. Please start the system first.${NC}"
    echo "Run: make up"
    exit 1
fi

if ! curl -s http://localhost:3000 >/dev/null 2>&1; then
    echo -e "${RED}Error: Dashboard is not accessible.${NC}"
    echo "Run: make up"
    exit 1
fi

echo -e "${GREEN}System is ready for demo recording!${NC}"
echo
echo "This script will guide you through a demonstration sequence."
echo "Open your screen recording software and prepare to record."
echo
read -p "Press Enter when ready to start the demo..."

# Demo sequence
print_step "1" "Show normal operation"
print_action "Open dashboard at http://localhost:3000"
print_action "Show Bus Monitor with normal frame flow"
print_action "Point out Node Health showing NORMAL state"
print_action "Show Charts with steady frame rate"
print_wait 3

print_step "2" "Demonstrate fault injection - Drop Sensor"
print_action "Click 'Drop Sensor' button in Fault Injection panel"
print_action "Watch Node B state change to DEGRADED"
print_action "Show Bus Monitor - sensor frames stop appearing"
print_action "Point out watchdog timeout counter increasing"
print_wait 5

print_step "3" "Show recovery"
print_action "Wait for fault injection to end automatically"
print_action "Show Node B state returning to NORMAL"
print_action "Show recovery count incrementing"
print_action "Point out Bus Monitor showing frames resume"
print_wait 3

print_step "4" "Demonstrate bus flooding"
print_action "Click 'Flood Bus' button"
print_action "Show Charts with increased frame rate"
print_action "Point out error counters in System Metrics"
print_action "Show Bus Monitor with mixed frame types"
print_wait 3

print_step "5" "Show frame filtering"
print_action "Use Bus Monitor filter to show only Sensor frames"
print_action "Switch to Command frames filter"
print_action "Toggle between Hex and Decimal data display"
print_wait 2

print_step "6" "Demonstrate delay injection"
print_action "Click 'Delay Frames' button"
print_action "Show timestamp spread in Bus Monitor"
print_action "Point out late frame counters"
print_wait 3

print_step "7" "Show system metrics"
print_action "Highlight System Metrics panel"
print_action "Show frame rate, bus load, error rates"
print_action "Point out real-time updates"
print_wait 2

print_step "8" "Demonstrate reset"
print_action "Click 'Reset Node A' button"
print_action "Show temporary frame interruption"
print_action "Show automatic recovery"
print_wait 2

echo
echo -e "${GREEN}Demo sequence complete!${NC}"
echo
echo "Demo highlights covered:"
echo "✓ Normal CAN bus operation"
echo "✓ Fault injection capabilities"
echo "✓ Watchdog and state machine behavior"
echo "✓ Real-time monitoring dashboard"
echo "✓ Frame filtering and analysis"
echo "✓ System recovery mechanisms"
echo
echo "For a shorter 30-second demo, focus on:"
echo "1. Normal operation (5s)"
echo "2. Drop sensor fault (10s)"
echo "3. Recovery (10s)"
echo "4. Flood bus (5s)"
echo
echo "Screenshots for README:"
echo "- Dashboard overview"
echo "- Bus Monitor with frame flow"
echo "- Node Health showing NORMAL state"
echo "- Fault injection panel"
echo "- Charts showing metrics"

