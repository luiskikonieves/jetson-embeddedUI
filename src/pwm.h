/**
* Base hardware PWM control using Linux PWM sysfs interface.
* Other classes will inherit this functionality for specific PWM applications
* 
*/

#ifndef PWM_H
#define PWM_H

#include <string>
#include <stdexcept>
#include <fstream>

class PWM {
public:
    PWM(const std::string& port, int chip, int chan, int freqHz);
    virtual ~PWM();

    void setDutyCycle(float dutyNs);
    virtual void start();
    virtual void stop();

    static constexpr const char* PWM_BASE_DIR = "/sys/class/pwm";

protected:
    std::string port;
    int chipNum;
    int channel;
    int periodNs;  // Period in nanoseconds
    bool running;

    void exportPWM();
    void unexportPWM();
    void writeSysfs(const std::string& path, const std::string& value);

private:
};

#endif // PWM_H
