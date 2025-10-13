#pragma once

#include "can_interface.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

class SensorTasks {
private:
    // Sensor data
    struct SensorData {
        float temperature;
        float pressure;
        float humidity;
        uint16_t lightLevel;
        uint8_t status;
        uint32_t sequenceNumber;
    };
    
    SensorData sensorData;
    std::atomic<uint32_t> sequenceCounter{0};
    std::mt19937 rng;
    
    // Timing
    std::chrono::steady_clock::time_point lastTransmission;
    std::chrono::milliseconds transmissionInterval{10}; // 100Hz default
    
    // CAN IDs for different sensor data
    static constexpr uint32_t TEMP_ID = 0x100;
    static constexpr uint32_t PRESSURE_ID = 0x101;
    static constexpr uint32_t HUMIDITY_ID = 0x102;
    static constexpr uint32_t LIGHT_ID = 0x103;
    static constexpr uint32_t STATUS_ID = 0x104;
    static constexpr uint32_t SEQUENCE_ID = 0x105;
    
public:
    SensorTasks() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        // Initialize sensor data with realistic values
        sensorData.temperature = 25.0f;
        sensorData.pressure = 1013.25f;
        sensorData.humidity = 60.0f;
        sensorData.lightLevel = 500;
        sensorData.status = 0x01; // OK status
        sensorData.sequenceNumber = 0;
    }
    
    bool initialize();
    void update();
    void transmitFrames();
    
private:
    void updateSensorData();
    void transmitTemperature();
    void transmitPressure();
    void transmitHumidity();
    void transmitLight();
    void transmitStatus();
    void transmitSequence();
    
    // Helper functions
    float generateRealisticValue(float base, float variation);
    void packFloat(uint8_t* data, float value);
    void packUint16(uint8_t* data, uint16_t value);
};

