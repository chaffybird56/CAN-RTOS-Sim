#include "sensor_tasks.h"
#include <iostream>
#include <iomanip>
#include <cstring>

bool SensorTasks::initialize() {
    lastTransmission = std::chrono::steady_clock::now();
    std::cout << "Sensor tasks initialized with " 
              << transmissionInterval.count() << "ms interval" << std::endl;
    return true;
}

void SensorTasks::update() {
    updateSensorData();
}

void SensorTasks::transmitFrames() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - lastTransmission;
    
    if (elapsed >= transmissionInterval) {
        transmitTemperature();
        transmitPressure();
        transmitHumidity();
        transmitLight();
        transmitStatus();
        transmitSequence();
        
        lastTransmission = now;
        sequenceCounter++;
    }
}

void SensorTasks::updateSensorData() {
    // Simulate realistic sensor behavior with some variation
    sensorData.temperature = generateRealisticValue(25.0f, 2.0f);
    sensorData.pressure = generateRealisticValue(1013.25f, 5.0f);
    sensorData.humidity = generateRealisticValue(60.0f, 10.0f);
    sensorData.lightLevel = static_cast<uint16_t>(generateRealisticValue(500.0f, 100.0f));
    
    // Update sequence number
    sensorData.sequenceNumber = sequenceCounter.load();
    
    // Occasionally simulate sensor errors (1% chance)
    if ((rng() % 100) == 0) {
        sensorData.status = 0x02; // Error status
        std::cout << "Sensor error simulated!" << std::endl;
    } else {
        sensorData.status = 0x01; // OK status
    }
}

void SensorTasks::transmitTemperature() {
    uint8_t data[8];
    packFloat(data, sensorData.temperature);
    packFloat(data + 4, static_cast<float>(sensorData.sequenceNumber));
    
    CANFrame frame(TEMP_ID, data, 8);
    if (CANInterface::getInstance().sendFrame(frame)) {
        std::cout << "TX " << std::hex << TEMP_ID << std::dec 
                  << " T=" << std::fixed << std::setprecision(1) << sensorData.temperature 
                  << "°C seq=" << sensorData.sequenceNumber << std::endl;
    }
}

void SensorTasks::transmitPressure() {
    uint8_t data[8];
    packFloat(data, sensorData.pressure);
    packFloat(data + 4, static_cast<float>(sensorData.sequenceNumber));
    
    CANFrame frame(PRESSURE_ID, data, 8);
    if (CANInterface::getInstance().sendFrame(frame)) {
        std::cout << "TX " << std::hex << PRESSURE_ID << std::dec 
                  << " P=" << std::fixed << std::setprecision(2) << sensorData.pressure 
                  << "hPa seq=" << sensorData.sequenceNumber << std::endl;
    }
}

void SensorTasks::transmitHumidity() {
    uint8_t data[8];
    packFloat(data, sensorData.humidity);
    packFloat(data + 4, static_cast<float>(sensorData.sequenceNumber));
    
    CANFrame frame(HUMIDITY_ID, data, 8);
    if (CANInterface::getInstance().sendFrame(frame)) {
        std::cout << "TX " << std::hex << HUMIDITY_ID << std::dec 
                  << " H=" << std::fixed << std::setprecision(1) << sensorData.humidity 
                  << "% seq=" << sensorData.sequenceNumber << std::endl;
    }
}

void SensorTasks::transmitLight() {
    uint8_t data[8];
    packUint16(data, sensorData.lightLevel);
    packFloat(data + 2, static_cast<float>(sensorData.sequenceNumber));
    
    CANFrame frame(LIGHT_ID, data, 8);
    if (CANInterface::getInstance().sendFrame(frame)) {
        std::cout << "TX " << std::hex << LIGHT_ID << std::dec 
                  << " L=" << sensorData.lightLevel 
                  << " seq=" << sensorData.sequenceNumber << std::endl;
    }
}

void SensorTasks::transmitStatus() {
    uint8_t data[8];
    data[0] = sensorData.status;
    data[1] = 0x00; // Reserved
    packFloat(data + 2, static_cast<float>(sensorData.sequenceNumber));
    
    CANFrame frame(STATUS_ID, data, 8);
    if (CANInterface::getInstance().sendFrame(frame)) {
        std::cout << "TX " << std::hex << STATUS_ID << std::dec 
                  << " Status=" << (sensorData.status == 0x01 ? "OK" : "ERROR") 
                  << " seq=" << sensorData.sequenceNumber << std::endl;
    }
}

void SensorTasks::transmitSequence() {
    uint8_t data[8];
    packFloat(data, static_cast<float>(sensorData.sequenceNumber));
    packFloat(data + 4, static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()));
    
    CANFrame frame(SEQUENCE_ID, data, 8);
    if (CANInterface::getInstance().sendFrame(frame)) {
        std::cout << "TX " << std::hex << SEQUENCE_ID << std::dec 
                  << " Sequence=" << sensorData.sequenceNumber << std::endl;
    }
}

float SensorTasks::generateRealisticValue(float base, float variation) {
    std::uniform_real_distribution<float> dist(-variation, variation);
    return base + dist(rng);
}

void SensorTasks::packFloat(uint8_t* data, float value) {
    memcpy(data, &value, sizeof(float));
}

void SensorTasks::packUint16(uint8_t* data, uint16_t value) {
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
}

