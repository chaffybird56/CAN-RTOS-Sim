# CAN-RTOS-Sim

Two "RTOS-like" embedded nodes communicating over a simulated CAN bus with live fault-injection and a web HIL panel.

## Why It Matters

This project simulates real-world embedded automotive systems where multiple ECUs communicate over CAN buses. It demonstrates:
- **Fault tolerance**: How systems handle communication failures
- **Watchdog mechanisms**: Automatic recovery from degraded states  
- **Live monitoring**: Real-time visibility into bus traffic and node health
- **Fault injection**: Controlled testing of system resilience

## Features

- **Virtual CAN Bus**: SocketCAN vcan0 interface running in Docker (no physical hardware needed)
- **Two Simulated Nodes**: 
  - Sensor node (periodic data transmission)
  - Controller node (watchdog + state machine)
- **Fault Injection**: Drop, delay, flood, and reset capabilities
- **Live Dashboard**: Real-time frame monitoring, node health, and charts
- **Cross-Platform**: Works on M3 Macs via Docker's Linux VM

## Stack

- **CAN Interface**: SocketCAN, python-can
- **Backend**: FastAPI, WebSocket streaming
- **Frontend**: Next.js, real-time charts
- **Orchestration**: Docker Compose
- **Language**: C++ (nodes), Python (server), TypeScript (panel)

## Quickstart

### Option 1: One-Command Setup (Recommended)

```bash
./quickstart.sh
```

This script will:
- Check prerequisites (Docker)
- Build all images
- Start all services
- Verify system health
- Open the dashboard

### Option 2: Manual Setup

```bash
# 1. Build all images
docker compose -f ops/compose.yml build

# 2. Start the virtual CAN bus
docker compose -f ops/compose.yml up -d bus

# 3. Start nodes, server, and dashboard
docker compose -f ops/compose.yml up -d node_a node_b server panel

# 4. Open dashboard
open http://localhost:3000
```

### Option 3: Using Make Commands

```bash
make build
make up
make dashboard
```

## How It Works

```
┌─────────────┐    vcan0     ┌─────────────┐
│   Node A    │◄─────────────►│   Node B    │
│ (Sensor)    │               │(Controller) │
└─────────────┘               └─────────────┘
       │                             │
       └─────────────┐       ┌───────┘
                     ▼       ▼
              ┌─────────────────────┐
              │    FastAPI Server   │
              │  (CAN Monitor +     │
              │   Fault Injection)  │
              └─────────────────────┘
                       │
                       ▼
              ┌─────────────────────┐
              │   Next.js Dashboard │
              │   (Live Monitoring) │
              └─────────────────────┘
```

## Fault Injection Scenarios

| Fault Type | Expected Behavior | Observable Result |
|------------|------------------|-------------------|
| **Drop Sensor** | Node B watchdog expires → DEGRADED state | Controller enters degraded mode, logs recovery |
| **Delay Frames** | Increased latency → late frame counters | Timestamp spread in bus monitor |
| **Flood Bus** | Error counters rise, potential throttling | High frame rate, possible missed frames |
| **Reset Node A** | Node A restarts, temporary frame loss | Controller detects restart, logs state changes |

## Metrics

- **Message Rate**: Frames per second (moving average)
- **Error Frames**: Dropped/corrupted frame count
- **Watchdog Events**: State transitions and recovery time
- **Bus Load**: Percentage utilization

## Troubleshooting

### Common Issues

**Docker not running**
```bash
# Start Docker Desktop and wait for it to be ready
docker info
```

**Services not starting**
```bash
# Check service status
docker compose -f ops/compose.yml ps

# View logs
docker compose -f ops/compose.yml logs
```

**No CAN traffic**
```bash
# Check if vcan0 exists
docker compose -f ops/compose.yml exec bus ip link show vcan0

# Monitor CAN traffic
docker compose -f ops/compose.yml exec bus candump -l vcan0
```

**Dashboard not accessible**
```bash
# Check if panel is running
curl http://localhost:3000

# Check server API
curl http://localhost:8000/health
```

### Validation Script

Run the built-in validation script to check system health:

```bash
./scripts/validate_can.sh
```

### System Test

Run the integration test to verify everything works:

```bash
python3 test_system.py
```

## Potential Additions

- [ ] Renode/Zephyr integration for more realistic RTOS simulation
- [ ] SQLite logging for historical analysis
- [ ] Prometheus metrics + Grafana dashboards
- [ ] Multi-bus topologies (CAN-FD, LIN)

## License

MIT
