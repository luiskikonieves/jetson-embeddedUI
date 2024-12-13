#include "IO.h"
#include <iostream>

/**
 * @brief Base constructor for IO objects
 * 
 * @param name Unique identifier for this IO
 * @param config Configuration structure containing all IO parameters
 */
IO::IO(const std::string& name, const Config& config)
    : name(name)
    , config(config)
    , currentSetPoint(config.initialSetPoint) {
}

/**
 * @brief Sets the IO to a specific setpoint value
 * 
 * @param index Index into the setPoints vector
 * @note Derived classes must implement the actual hardware setting
 */
void IO::setPoint(size_t index) {
    if (index < config.setPoints.size()) {
        currentSetPoint = index;
        // Derived classes will implement the actual hardware setting
    }
}

/**
 * @brief Constructs a PWM-specific IO object
 * 
 * @param name Unique identifier for this PWM IO
 * @param config Configuration structure containing PWM parameters
 * @throws std::runtime_error if PWM initialization fails
 */
PWMIO::PWMIO(const std::string& name, const Config& config)
    : IO(name, config) {
    if (config.isEnabled) {
        try {
            int chipNum = std::stoi(config.port.substr(7));  // Extract from "pwmchip0"
            pwm = std::make_unique<PWM>(config.port, chipNum, 0, PWM_FREQUENCY_HZ);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create PWM: " << e.what() << std::endl;
            throw;
        }
    }
}

/**
 * @brief Starts the PWM output
 * 
 * Initializes the PWM hardware and sets it to the initial setpoint
 * @throws std::runtime_error if PWM start fails
 */
void PWMIO::start() {
    if (config.isEnabled && pwm) {
        pwm->start();
        setPoint(currentSetPoint);
    }
}

/**
 * @brief Stops the PWM output
 * 
 * Disables the PWM hardware output
 * @throws std::runtime_error if PWM stop fails
 */
void PWMIO::stop() {
    if (config.isEnabled && pwm) {
        pwm->stop();
    }
}

/**
 * @brief Reads the current PWM value
 * 
 * @return float Current setpoint value
 * @note Currently returns the setpoint value; hardware feedback could be implemented here
 */
float PWMIO::read() const {
    // Implement PWM feedback reading if hardware supports it
    return config.setPoints[currentSetPoint];
}

/**
 * @brief Initializes all IO devices from configuration settings
 * 
 * Creates and starts all enabled IO devices based on their configuration
 * @param ioSettings Map of IO configurations from settings file
 */
void IOManager::initialize(const std::map<std::string, Settings::IO>& ioSettings) {
    for (const auto& [name, settings] : ioSettings) {
        // Skip IO11 as it's managed by SgcuManager
        if (name == "IO11") continue;
        
        auto io = createIO(name, settings);
        if (io) {
            try {
                ios[name] = std::move(io);
                auto* ptr = ios[name].get();
                if (ptr && ptr->isEnabled()) {
                    ptr->start();
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception during IO initialization for " << name << ": " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Failed to create IO for: " << name << std::endl;
        }
    }
}

/**
 * @brief Creates appropriate IO object based on configuration
 * 
 * Factory method that creates either PWM or GPIO IO objects
 * @param name Unique identifier for the IO
 * @param settings Configuration settings for the IO
 * @return std::unique_ptr<IO> Pointer to created IO object
 */
std::unique_ptr<IO> IOManager::createIO(const std::string& name, const Settings::IO& settings) {
    IO::Config config;
    config.pinNumber = static_cast<int>(settings.pinNumber);
    config.port = settings.port;
    config.type = (settings.pinFunction == "PWM") ? IO::Type::PWM : IO::Type::GPIO;
    config.name = settings.pinName;
    config.isEnabled = settings.isEnabled;
    config.setPoints.assign(settings.setPoints.begin(), settings.setPoints.end());
    config.initialSetPoint = settings.initialValue;

    if (config.type == IO::Type::PWM) {
        try {
            return std::make_unique<PWMIO>(name, config);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create PWM IO: " << e.what() << std::endl;
            return nullptr;
        }
    } else {
        try {
            return std::make_unique<GPIIO>(name, config);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create GPIO IO: " << e.what() << std::endl;
            return nullptr;
        }
    }
}

IOManager& IOManager::getInstance() {
    static IOManager instance;
    return instance;
}

// Add getIO implementation
IO* IOManager::getIO(const std::string& name) {
    auto it = ios.find(name);
    return (it != ios.end()) ? it->second.get() : nullptr;
}

// Add GPIIO constructor implementation
GPIIO::GPIIO(const std::string& name, const Config& config)
    : IO(name, config) {
    // TODO: Initialize GPIO hardware
}

void GPIIO::start() {
    // TODO: Implement GPIO start logic
}

void GPIIO::stop() {
    // TODO: mplement GPIO stop logic
}

float GPIIO::read() const {
    // TODO: Implement GPIO read logic
    return 0.0f;  // Placeholder return
}

void PWMIO::setPoint(size_t index) {
    if (index < config.setPoints.size()) {
        currentSetPoint = index;
        if (config.isEnabled && pwm) {
            // Convert setpoint from microseconds to nanoseconds (multiply by 1000)
            float valueNs = config.setPoints[index] * 1000;
            pwm->setDutyCycle(valueNs);
        }
    }
}
