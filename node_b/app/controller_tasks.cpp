#include "controller_tasks.h"
#include <iostream>
#include <iomanip>
#include <cstring>

bool ControllerTasks::initialize() {
    std::cout << "Controller tasks initialized" << std::endl;
    std::cout << "Watchdog timeout: " << watchdog.timeout.count() << "ms" << std::endl;
    std::cout << "Temperature thresholds: " << TEMP_LOW_THRESHOLD << "°C - " << TEMP_HIGH_THRESHOLD << "°C" << std::endl;
    std::cout << "Pressure thresholds: " << PRESSURE_LOW_THRESHOLD << "hPa - " << PRESSURE_HIGH_THRESHOLD << "hPa" << std::endl;
    return true;
}

void ControllerTasks::processReceivedFrames() {
    CANFrame frame;
    while (CANInterface::getInstance().receiveFrame(frame)) {
        framesReceived++;
        processSensorFrame(frame);
    }
}

void ControllerTasks::updateWatchdog() {
    std::lock_guard<std::mutex> lock(watchdogMutex);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - watchdog.lastHeartbeat;
    
    bool wasExpired = watchdog.expired;
    watchdog.expired = (elapsed > watchdog.timeout);
    
    if (watchdog.expired && !wasExpired) {
        watchdog.timeoutCount++;
        std::cout << "WATCHDOG EXPIRED! Timeout count: " << watchdog.timeoutCount << std::endl;
    }
}

void ControllerTasks::updateStateMachine() {
    std::lock_guard<std::mutex> sensorLock(sensorDataMutex);
    std::lock_guard<std::mutex> watchdogLock(watchdogMutex);
    
    NodeState newState = currentState;
    
    switch (currentState) {
        case NodeState::NORMAL:
            if (watchdog.expired || sensorData.status != 0x01) {
                newState = NodeState::DEGRADED;
            }
            break;
            
        case NodeState::DEGRADED:
            if (!watchdog.expired && sensorData.status == 0x01) {
                newState = NodeState::RECOVERING;
            } else if (watchdog.timeoutCount > 5) {
                newState = NodeState::FAILED;
            }
            break;
            
        case NodeState::RECOVERING:
            // Stay in recovering for a short time to ensure stability
            if (watchdog.expired) {
                newState = NodeState::DEGRADED;
            } else {
                // Check if we've been stable for a while
                auto now = std::chrono::steady_clock::now();
                auto elapsed = now - sensorData.lastUpdate;
                if (elapsed < std::chrono::milliseconds(100)) {
                    newState = NodeState::NORMAL;
                    watchdog.recoveryCount++;
                    std::cout << "RECOVERED! Recovery count: " << watchdog.recoveryCount << std::endl;
                }
            }
            break;
            
        case NodeState::FAILED:
            // Failed state requires manual intervention (could be reset via API)
            break;
    }
    
    if (newState != currentState) {
        transitionToState(newState);
    }
    
    // Check thresholds and send commands if needed
    checkThresholds();
}

void ControllerTasks::transmitCommands() {
    std::lock_guard<std::mutex> lock(commandQueueMutex);
    
    while (!commandQueue.empty()) {
        CANFrame frame = commandQueue.front();
        commandQueue.pop();
        
        if (CANInterface::getInstance().sendFrame(frame)) {
            commandsSent++;
            std::cout << "TX CMD " << std::hex << frame.id << std::dec 
                      << " length=" << static_cast<int>(frame.length) << std::endl;
        }
    }
}

std::string ControllerTasks::getStateString() const {
    switch (currentState) {
        case NodeState::NORMAL: return "NORMAL";
        case NodeState::DEGRADED: return "DEGRADED";
        case NodeState::RECOVERING: return "RECOVERING";
        case NodeState::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

uint32_t ControllerTasks::getTimeoutCount() const {
    std::lock_guard<std::mutex> lock(watchdogMutex);
    return watchdog.timeoutCount;
}

uint32_t ControllerTasks::getRecoveryCount() const {
    std::lock_guard<std::mutex> lock(watchdogMutex);
    return watchdog.recoveryCount;
}

SensorData ControllerTasks::getSensorData() const {
    std::lock_guard<std::mutex> lock(sensorDataMutex);
    return sensorData;
}

void ControllerTasks::processSensorFrame(const CANFrame& frame) {
    std::lock_guard<std::mutex> lock(sensorDataMutex);
    std::lock_guard<std::mutex> watchdogLock(watchdogMutex);
    
    // Update watchdog on any sensor frame
    watchdog.lastHeartbeat = std::chrono::steady_clock::now();
    
    // Check for late frames
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - watchdog.lastHeartbeat;
    if (elapsed > std::chrono::milliseconds(50)) {
        lateFrames++;
    }
    
    updateSensorData(frame.id, frame.data, frame.length);
    
    std::cout << "RX " << std::hex << frame.id << std::dec 
              << " length=" << static_cast<int>(frame.length) << std::endl;
}

void ControllerTasks::updateSensorData(uint32_t id, const uint8_t* data, uint8_t length) {
    auto now = std::chrono::steady_clock::now();
    
    switch (id) {
        case TEMP_ID:
            if (length >= 4) {
                sensorData.temperature = unpackFloat(data);
                sensorData.sequenceNumber = static_cast<uint32_t>(unpackFloat(data + 4));
            }
            break;
            
        case PRESSURE_ID:
            if (length >= 4) {
                sensorData.pressure = unpackFloat(data);
            }
            break;
            
        case HUMIDITY_ID:
            if (length >= 4) {
                sensorData.humidity = unpackFloat(data);
            }
            break;
            
        case LIGHT_ID:
            if (length >= 2) {
                sensorData.lightLevel = unpackUint16(data);
            }
            break;
            
        case STATUS_ID:
            if (length >= 1) {
                sensorData.status = data[0];
                if (sensorData.status != 0x01) {
                    errorFrames++;
                }
            }
            break;
            
        case SEQUENCE_ID:
            if (length >= 4) {
                sensorData.sequenceNumber = static_cast<uint32_t>(unpackFloat(data));
            }
            break;
    }
    
    sensorData.lastUpdate = now;
    sensorData.valid = true;
}

void ControllerTasks::checkThresholds() {
    std::lock_guard<std::mutex> lock(sensorDataMutex);
    
    if (!sensorData.valid) return;
    
    uint8_t cmdData[8];
    
    // Temperature threshold checks
    if (sensorData.temperature > TEMP_HIGH_THRESHOLD) {
        packFloat(cmdData, sensorData.temperature);
        packFloat(cmdData + 4, TEMP_HIGH_THRESHOLD);
        sendCommand(TEMP_THRESHOLD_CMD, cmdData, 8);
        std::cout << "Temperature HIGH threshold exceeded: " << sensorData.temperature << "°C" << std::endl;
    } else if (sensorData.temperature < TEMP_LOW_THRESHOLD) {
        packFloat(cmdData, sensorData.temperature);
        packFloat(cmdData + 4, TEMP_LOW_THRESHOLD);
        sendCommand(TEMP_THRESHOLD_CMD, cmdData, 8);
        std::cout << "Temperature LOW threshold exceeded: " << sensorData.temperature << "°C" << std::endl;
    }
    
    // Pressure threshold checks
    if (sensorData.pressure > PRESSURE_HIGH_THRESHOLD) {
        packFloat(cmdData, sensorData.pressure);
        packFloat(cmdData + 4, PRESSURE_HIGH_THRESHOLD);
        sendCommand(PRESSURE_THRESHOLD_CMD, cmdData, 8);
        std::cout << "Pressure HIGH threshold exceeded: " << sensorData.pressure << "hPa" << std::endl;
    } else if (sensorData.pressure < PRESSURE_LOW_THRESHOLD) {
        packFloat(cmdData, sensorData.pressure);
        packFloat(cmdData + 4, PRESSURE_LOW_THRESHOLD);
        sendCommand(PRESSURE_THRESHOLD_CMD, cmdData, 8);
        std::cout << "Pressure LOW threshold exceeded: " << sensorData.pressure << "hPa" << std::endl;
    }
    
    // Emergency condition
    if (currentState == NodeState::FAILED) {
        cmdData[0] = 0xFF; // Emergency code
        cmdData[1] = static_cast<uint8_t>(currentState);
        sendCommand(EMERGENCY_CMD, cmdData, 8);
        std::cout << "EMERGENCY COMMAND SENT!" << std::endl;
    }
}

void ControllerTasks::sendCommand(uint32_t id, const uint8_t* data, uint8_t length) {
    std::lock_guard<std::mutex> lock(commandQueueMutex);
    commandQueue.push(CANFrame(id, data, length));
}

void ControllerTasks::transitionToState(NodeState newState) {
    std::cout << "State transition: " << getStateString() << " -> ";
    currentState = newState;
    stateTransitionCount++;
    std::cout << getStateString() << " (transition #" << stateTransitionCount << ")" << std::endl;
}

float ControllerTasks::unpackFloat(const uint8_t* data) {
    float value;
    memcpy(&value, data, sizeof(float));
    return value;
}

uint16_t ControllerTasks::unpackUint16(const uint8_t* data) {
    return data[0] | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t ControllerTasks::unpackUint32(const uint8_t* data) {
    return data[0] | (static_cast<uint32_t>(data[1]) << 8) | 
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

void ControllerTasks::packFloat(uint8_t* data, float value) {
    memcpy(data, &value, sizeof(float));
}

void ControllerTasks::packUint16(uint8_t* data, uint16_t value) {
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
}

