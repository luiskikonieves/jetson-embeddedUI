#include "pwm.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <chrono>

/**
 * @brief Constructs a PWM object and initializes the PWM hardware
 * 
 * @param port The PWM port identifier (e.g., "pwmchip0")
 * @param chip The PWM chip number to use
 * @param channel The PWM channel on the specified chip
 * @param freqHz The PWM frequency in Hz
 * @throws std::runtime_error if PWM initialization fails
 */
PWM::PWM(const std::string& port, int chip, int channel, int freqHz) 
    : port(port),
      chipNum(chip),
      channel(channel),
      periodNs(1000000000 / freqHz),  // Convert Hz to ns
      running(false) {
    
    try {
        exportPWM();
        std::cout << "PWM initialized on port " << port 
                  << " (chip " << chipNum << ", channel " << channel 
                  << ") at " << freqHz << "Hz" << std::endl;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize PWM on port " + 
                               port + ": " + e.what());
    }
}

/**
 * @brief Destructor that ensures proper cleanup of PWM resources
 */
PWM::~PWM() {
    try {
        if (running) {
            stop();
        }
        unexportPWM();
    } catch (const std::exception& e) {
        std::cerr << "Error during PWM cleanup: " << e.what() << std::endl;
    }
}

/**
 * @brief Starts PWM output
 * @throws std::runtime_error if unable to enable PWM
 */
void PWM::start() {
    if (!running) {
        try {
            std::string path = std::string(PWM_BASE_DIR) + "/pwmchip" + 
                              std::to_string(chipNum) + "/pwm" + 
                              std::to_string(channel) + "/enable";
            writeSysfs(path, "1");
            running = true;
            std::cout << "PWM started on port " << port << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to start PWM: " + std::string(e.what()));
        }
    }
}

/**
 * @brief Stops PWM output
 * @throws std::runtime_error if unable to disable PWM
 */
void PWM::stop() {
    if (running) {
        try {
            std::string path = std::string(PWM_BASE_DIR) + "/pwmchip" + 
                              std::to_string(chipNum) + "/pwm" + 
                              std::to_string(channel) + "/enable";
            writeSysfs(path, "0");
            running = false;
            std::cout << "PWM stopped on port " << port << std::endl;
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to stop PWM: " + std::string(e.what()));
        }
    }
}

/**
 * @brief Sets the PWM duty cycle
 * 
 * @param dutyNs The duty cycle value in nanoseconds
 * @throws std::runtime_error if unable to set duty cycle value
 */
void PWM::setDutyCycle(float dutyNs) {
    try {
        std::string path = std::string(PWM_BASE_DIR) + "/pwmchip" + 
                          std::to_string(chipNum) + "/pwm" + 
                          std::to_string(channel) + "/duty_cycle";
        writeSysfs(path, std::to_string(static_cast<int>(dutyNs)));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to set duty cycle: " + std::string(e.what()));
    }
}

/**
 * @brief Exports and initializes a PWM channel in the sysfs interface 
 * @throws std::runtime_error if PWM export fails or if unable to write initial settings
 */
void PWM::exportPWM() {
    // Correct path structure: /sys/class/pwm/pwmchipX/...
    std::string chipDir = PWM_BASE_DIR + std::string("/pwmchip") + 
                         std::to_string(chipNum);
    
    std::string pwmDir = chipDir + "/pwm" + std::to_string(channel);
    
    if (!std::filesystem::exists(pwmDir)) {
        // Export the PWM if it doesn't exist
        writeSysfs(chipDir + "/export", std::to_string(channel));
        
        // Wait briefly for the export to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (!std::filesystem::exists(pwmDir)) {
            throw std::runtime_error("Failed to export PWM - directory not created: " + pwmDir);
        }
    }
    
    // Configure the period first (required before duty cycle)
    writeSysfs(pwmDir + "/period", std::to_string(periodNs));
    
    // Set initial duty cycle to 0
    writeSysfs(pwmDir + "/duty_cycle", "0");
    
    // Ensure PWM starts disabled
    writeSysfs(pwmDir + "/enable", "0");
}

/**
 * @brief Unexports a PWM channel from the sysfs interface
 * @throws std::runtime_error if unable to unexport the PWM channel (error is caught and logged)
 */
void PWM::unexportPWM() {
    try {
        std::string path = std::string(PWM_BASE_DIR) + "/pwmchip" + 
                          std::to_string(chipNum) + "/unexport";
        writeSysfs(path, std::to_string(channel));
    } catch (const std::exception& e) {
        std::cerr << "Error unexporting PWM: " << e.what() << std::endl;
    }
}

/**
 * @brief Writes a value to a sysfs file in the Linux filesystem
 * 
 * @param path Full path to the sysfs file to write to
 * @param value String value to write to the file
 * @throws std::runtime_error if unable to open or write to the sysfs file
 */
void PWM::writeSysfs(const std::string& path, const std::string& value) {
    std::cout << "Writing to " << path << ": " << value << std::endl; 
    std::ofstream fs(path);
    if (!fs.is_open()) {
        throw std::runtime_error("Failed to open sysfs file: " + path);
    }
    fs << value;
    fs.close();
    
    if (!fs) {
        throw std::runtime_error("Failed to write to sysfs file: " + path);
    }
}