/**
* Programmable inputs and outputs. Allows the user to define IO behavior via the UI. 
*/

#ifndef IO_H
#define IO_H

#include <string>
#include <vector>
#include <memory>
#include "pwm.h"
#include "configuration.hpp"

class IO {
public:
    enum class Type {
        PWM,
        GPIO
    };

    enum class Direction {
        INPUT,
        OUTPUT
    };

    struct Config {
        uint8_t pinNumber;
        std::string port;
        Type type;
        Direction direction;
        std::string name;
        bool isEnabled;
        std::vector<float> setPoints;  // Multiple setpoints stored as vector
        size_t initialSetPoint;        // Index into setPoints vector
    };

    IO(const std::string& name, const Config& config);
    virtual ~IO() = default;

    // Common interface
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setPoint(size_t index);  // Set to specific setPoint by index
    virtual float read() const = 0;       // Read current value
    
    const std::string& getName() const { return name; }
    bool isEnabled() const { return config.isEnabled; }
    size_t getCurrentSetPoint() const { return currentSetPoint; }
    size_t getSetPointCount() const { return config.setPoints.size(); }
    Type getType() const { return config.type; }
    Direction getDirection() const { return config.direction; }

protected:
    std::string name;
    Config config;
    size_t currentSetPoint;
};

// PWM-specific implementation
class PWMIO : public IO {
public:
    PWMIO(const std::string& name, const Config& config);
    
    void start() override;
    void stop() override;
    void setPoint(size_t index) override;
    float read() const override;

private:
    std::unique_ptr<PWM> pwm;
    static constexpr int PWM_FREQUENCY_HZ = 50;
};

// GPIO-specific implementation
class GPIIO : public IO {
public:
    GPIIO(const std::string& name, const Config& config);
    
    void start() override;
    void stop() override;
    float read() const override;

private:
    // GPIO specific members
};

// Factory class to manage IOs
class IOManager {
public:
    static IOManager& getInstance();
    void initialize(const std::map<std::string, Settings::IO>& ioSettings);
    IO* getIO(const std::string& name);
    std::vector<IO*> getIOsByType(IO::Type type);

private:
    IOManager() = default;
    std::map<std::string, std::unique_ptr<IO>> ios;
    
    std::unique_ptr<IO> createIO(const std::string& name, const Settings::IO& settings);
};

#endif // IO_H
