# Media Assets

This directory contains media assets for the CAN-RTOS-Sim project.

## Directory Structure

- `hero.gif` - Main demonstration video (30-60 seconds)
- `screenshots/` - Dashboard screenshots for documentation

## Screenshots to Capture

For the README and documentation, capture these screenshots:

1. **Dashboard Overview** - Full dashboard showing all panels
2. **Bus Monitor** - Active frame monitoring with various frame types
3. **Node Health** - Nodes in NORMAL state with green indicators
4. **Fault Injection** - Panel showing available fault types
5. **Charts** - Real-time metrics and frame rate visualization
6. **Degraded State** - Node B in DEGRADED state after fault injection
7. **Recovery** - Node B returning to NORMAL state

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

