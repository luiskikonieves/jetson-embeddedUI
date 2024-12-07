/**
* Basic driver for serial devices. 
*/

#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <string>
#include <termios.h>

class Serial {
protected:
    int fd; // File descriptor for the serial port

public:
    Serial(){}
    Serial(const std::string& port, speed_t baud_rate);
    virtual ~Serial();

    void writeBytestream(const void* data, size_t size);
    void writeString(const std::string& str);
    void writeASCII(const char* asciiData, size_t length);
    std::string read(); 
    static const std::string deviceDirectory; // Jetson Linux file system directory for devices/peripherals 
};

#endif // SERIALMANAGER_H
