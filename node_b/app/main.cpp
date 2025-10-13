#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <iomanip>
#include "can_interface.h"
#include "controller_tasks.h"

std::atomic<bool> running{true};

void signalHandler(int signum) {
    std::cout << "Received signal " << signum << ", shutting down..." << std::endl;
    running = false;
}

int main() {
    std::cout << "=== CAN-RTOS-Sim Node B (Controller) ===" << std::endl;
    
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize CAN interface
    std::string canInterface = getenv("CAN_INTERFACE") ? getenv("CAN_INTERFACE") : "vcan0";
    if (!CANInterface::getInstance().initialize(canInterface)) {
        std::cerr << "Failed to initialize CAN interface: " << canInterface << std::endl;
        return 1;
    }
    
    std::cout << "CAN interface initialized: " << canInterface << std::endl;
    
    // Initialize controller tasks
    ControllerTasks controllerTasks;
    if (!controllerTasks.initialize()) {
        std::cerr << "Failed to initialize controller tasks" << std::endl;
        return 1;
    }
    
    std::cout << "Controller tasks initialized" << std::endl;
    
    // Main loop
    std::cout << "Starting controller node main loop..." << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    int frameCount = 0;
    
    while (running) {
        auto loopStart = std::chrono::steady_clock::now();
        
        // Process received frames
        controllerTasks.processReceivedFrames();
        
        // Update watchdog and state machine
        controllerTasks.updateWatchdog();
        controllerTasks.updateStateMachine();
        
        // Transmit command frames if needed
        controllerTasks.transmitCommands();
        
        frameCount++;
        
        // Log status every 100 iterations
        if (frameCount % 100 == 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            double rate = frameCount / static_cast<double>(elapsed);
            std::cout << "Node B: " << frameCount << " iterations, " 
                      << std::fixed << std::setprecision(2) << rate << " Hz" << std::endl;
        }
        
        // Sleep to maintain timing
        auto loopEnd = std::chrono::steady_clock::now();
        auto loopDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loopEnd - loopStart);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5) - loopDuration);
    }
    
    std::cout << "Shutting down Node B..." << std::endl;
    CANInterface::getInstance().cleanup();
    
    return 0;
}
