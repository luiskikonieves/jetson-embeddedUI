#include "configuration.hpp"
#include "UiServer.h"
#include "IO.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <filesystem>

int main() {
    // Initialize settings from configuration file
    Settings settings("configuration/settings.json");
    int port = settings.serverSettings.port;

    // Initialize UI server
    UiServer uiServer;
    if (!uiServer.initialize(port)) {
        std::cerr << "Failed to initialize UiServer." << std::endl;
        return -1;
    }

    // Initialize timing variables
    auto lastServiceTime = std::chrono::steady_clock::now();
    const auto uiServiceInterval = std::chrono::milliseconds(1000);

    // Initialize IOManager and configure PWM pins
    IOManager& ioManager = IOManager::getInstance();
    try {
        ioManager.initialize(settings.ioSettings);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize IOManager: " << e.what() << std::endl;
        return -1;
    }

    // Configure PWM pins
    std::string pwmIOKey;
    try {
        pwmIOKey = settings.findIOKeyByPinName("pwm0");
        if (!pwmIOKey.empty()) {
            IO* pwmIO = ioManager.getIO(pwmIOKey);
            if (!pwmIO) {
                std::cerr << "Failed to get PWM IO object" << std::endl;
                pwmIOKey = "";
            }
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Error initializing PWM: " << e.what() << std::endl;
        pwmIOKey = "";
    }

    // Set up PWM control callback
    uiServer.setPwmControlCallback([&ioManager, &pwmIOKey](double setpoint) {
        std::cout << "PWM control callback triggered with setpoint: " << setpoint << std::endl;
        if (!pwmIOKey.empty()) {
            IO* pwmIO = ioManager.getIO(pwmIOKey);
            if (pwmIO) {
                pwmIO->setPoint(setpoint);
            }
        }
    });

    // Main service loop
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();

        // Service the UI
        if (currentTime - lastServiceTime >= uiServiceInterval) {
            std::cout << "Calling UiServer::service()" << std::endl;
            uiServer.service();
            lastServiceTime = currentTime;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
