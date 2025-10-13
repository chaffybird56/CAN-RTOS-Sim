#include "can_interface.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

CANInterface* CANInterface::instance = nullptr;

bool CANInterface::initialize(const std::string& iface) {
    std::lock_guard<std::mutex> lock(interfaceMutex);
    
    if (initialized) {
        return true;
    }
    
    interfaceName = iface;
    
    // Create socket
    socketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socketFd < 0) {
        std::cerr << "Error creating CAN socket" << std::endl;
        return false;
    }
    
    // Get interface index
    struct ifreq ifr;
    strcpy(ifr.ifr_name, iface.c_str());
    if (ioctl(socketFd, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Error getting interface index for " << iface << std::endl;
        close(socketFd);
        socketFd = -1;
        return false;
    }
    
    // Bind socket to interface
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(socketFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding socket to " << iface << std::endl;
        close(socketFd);
        socketFd = -1;
        return false;
    }
    
    initialized = true;
    std::cout << "CAN interface " << iface << " initialized successfully" << std::endl;
    return true;
}

void CANInterface::cleanup() {
    std::lock_guard<std::mutex> lock(interfaceMutex);
    
    if (socketFd >= 0) {
        close(socketFd);
        socketFd = -1;
    }
    
    initialized = false;
    std::cout << "CAN interface cleaned up" << std::endl;
}

bool CANInterface::sendFrame(const CANFrame& frame) {
    std::lock_guard<std::mutex> lock(interfaceMutex);
    
    if (!initialized || socketFd < 0) {
        return false;
    }
    
    struct can_frame canFrame;
    canFrame.can_id = frame.id;
    canFrame.can_dlc = frame.length;
    
    for (int i = 0; i < frame.length && i < 8; i++) {
        canFrame.data[i] = frame.data[i];
    }
    
    ssize_t bytesSent = write(socketFd, &canFrame, sizeof(canFrame));
    if (bytesSent < 0) {
        std::cerr << "Error sending CAN frame" << std::endl;
        return false;
    }
    
    return true;
}

bool CANInterface::receiveFrame(CANFrame& frame) {
    std::lock_guard<std::mutex> lock(interfaceMutex);
    
    if (!initialized || socketFd < 0) {
        return false;
    }
    
    struct can_frame canFrame;
    ssize_t bytesRead = read(socketFd, &canFrame, sizeof(canFrame));
    
    if (bytesRead < 0) {
        return false;
    }
    
    frame.id = canFrame.can_id;
    frame.length = canFrame.can_dlc;
    frame.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (int i = 0; i < frame.length && i < 8; i++) {
        frame.data[i] = canFrame.data[i];
    }
    
    return true;
}

