#pragma once

#include "can_interface.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <queue>
#include <mutex>

enum class NodeState {
    NORMAL,
    DEGRADED,
    RECOVERING,
    FAILED
};

struct SensorData {
    float temperature;
    float pressure;
    float humidity;
    uint16_t lightLevel;
    uint8_t status;
    uint32_t sequenceNumber;
    std::chrono::steady_clock::time_point lastUpdate;
    bool valid;
    
    SensorData() : temperature(0), pressure(0), humidity(0), lightLevel(0), 
                   status(0), sequenceNumber(0), valid(false) {}
};

struct WatchdogData {
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::chrono::milliseconds timeout;
    bool expired;
    uint32_t timeoutCount;
    uint32_t recoveryCount;
    
    WatchdogData() : timeout(2000), expired(false), timeoutCount(0), recoveryCount(0) {}
};

class ControllerTasks {
private:
    // State management
    NodeState currentState;
    std::atomic<uint32_t> stateTransitionCount{0};
    
    // Sensor data storage
    SensorData sensorData;
    mutable std::mutex sensorDataMutex;
    
    // Watchdog management
    WatchdogData watchdog;
    mutable std::mutex watchdogMutex;
    
    // Command transmission
    std::queue<CANFrame> commandQueue;
    std::mutex commandQueueMutex;
    
    // Statistics
    std::atomic<uint32_t> framesReceived{0};
    std::atomic<uint32_t> commandsSent{0};
    std::atomic<uint32_t> lateFrames{0};
    std::atomic<uint32_t> errorFrames{0};
    
    // CAN IDs
    static constexpr uint32_t TEMP_ID = 0x100;
    static constexpr uint32_t PRESSURE_ID = 0x101;
    static constexpr uint32_t HUMIDITY_ID = 0x102;
    static constexpr uint32_t LIGHT_ID = 0x103;
    static constexpr uint32_t STATUS_ID = 0x104;
    static constexpr uint32_t SEQUENCE_ID = 0x105;
    
    // Command IDs
    static constexpr uint32_t TEMP_THRESHOLD_CMD = 0x200;
    static constexpr uint32_t PRESSURE_THRESHOLD_CMD = 0x201;
    static constexpr uint32_t EMERGENCY_CMD = 0x2FF;
    
    // Thresholds
    static constexpr float TEMP_HIGH_THRESHOLD = 30.0f;
    static constexpr float TEMP_LOW_THRESHOLD = 15.0f;
    static constexpr float PRESSURE_HIGH_THRESHOLD = 1020.0f;
    static constexpr float PRESSURE_LOW_THRESHOLD = 1000.0f;
    
public:
    ControllerTasks() : currentState(NodeState::NORMAL) {}
    
    bool initialize();
    void processReceivedFrames();
    void updateWatchdog();
    void updateStateMachine();
    void transmitCommands();
    
    // State accessors
    NodeState getCurrentState() const { return currentState; }
    std::string getStateString() const;
    uint32_t getStateTransitionCount() const { return stateTransitionCount.load(); }
    
    // Statistics accessors
    uint32_t getFramesReceived() const { return framesReceived.load(); }
    uint32_t getCommandsSent() const { return commandsSent.load(); }
    uint32_t getLateFrames() const { return lateFrames.load(); }
    uint32_t getErrorFrames() const { return errorFrames.load(); }
    uint32_t getTimeoutCount() const;
    uint32_t getRecoveryCount() const;
    
    // Sensor data accessors
    SensorData getSensorData() const;
    
private:
    void processSensorFrame(const CANFrame& frame);
    void updateSensorData(uint32_t id, const uint8_t* data, uint8_t length);
    void checkThresholds();
    void sendCommand(uint32_t id, const uint8_t* data, uint8_t length);
    void transitionToState(NodeState newState);
    
    // Helper functions
    float unpackFloat(const uint8_t* data);
    uint16_t unpackUint16(const uint8_t* data);
    uint32_t unpackUint32(const uint8_t* data);
    void packFloat(uint8_t* data, float value);
    void packUint16(uint8_t* data, uint16_t value);
};
