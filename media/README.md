# Media Assets

This directory contains media assets for the CAN-RTOS-Sim project.

## Directory Structure

- `hero.gif` - Main demonstration video (30-60 seconds)
- `screenshots/` - Dashboard screenshots for documentation

## Screenshots (in repo)

Captured for the README under `screenshots/`:

| File | Shows |
|------|--------|
| `dashboard-overview.png` | Full HIL dashboard — nodes, fault panel, charts, bus monitor |
| `fault-injection-active.png` | After **Drop Sensor** — watchdog timeouts and recovery count |
| `bus-monitor.png` | Live CAN table with sensor/command frames and filters |
| `flood-bus-charts.png` | **Flood Bus** fault — frame-rate spike and bus load |

## Demo Video Script

The `hero.gif` should demonstrate:

1. **0-5s**: Normal operation, show steady frame flow
2. **5-15s**: Inject drop fault, show state transition to DEGRADED
3. **15-25s**: Show recovery, state returns to NORMAL
4. **25-35s**: Inject flood fault, show error counters rising
5. **35-45s**: Show frame filtering and bus monitor features
6. **45-60s**: Return to normal operation, highlight key metrics

## File Naming Convention

- Screenshots: `dashboard-overview.png`, `bus-monitor.png`, etc.
- Video: `hero.gif` (keep under 10MB for GitHub)

