#pragma once

#include <string>
#include <vector>
#include <mutex>

struct CANFrame {
    uint32_t id;
    uint8_t data[8];
    uint8_t length;
    uint64_t timestamp;
    
    CANFrame(uint32_t canId = 0, const uint8_t* payload = nullptr, uint8_t len = 0) 
        : id(canId), length(len), timestamp(0) {
        if (payload && len <= 8) {
            for (int i = 0; i < len; i++) {
                data[i] = payload[i];
            }
        }
    }
};

class CANInterface {
private:
    static CANInterface* instance;
    std::string interfaceName;
    int socketFd;
    bool initialized;
    mutable std::mutex interfaceMutex;
    
    CANInterface() : socketFd(-1), initialized(false) {}
    
public:
    static CANInterface& getInstance() {
        if (!instance) {
            instance = new CANInterface();
        }
        return *instance;
    }
    
    bool initialize(const std::string& iface);
    void cleanup();
    bool sendFrame(const CANFrame& frame);
    bool receiveFrame(CANFrame& frame);
    bool isInitialized() const { return initialized; }
    std::string getInterfaceName() const { return interfaceName; }
};

