#include "serial.h"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

const std::string Serial::deviceDirectory = "/dev";

/**
 * Constructor for Serial object.
 * 
 * @param port The serial port device file name (e.g. /ttyUSB0).
 * @param baudRate Serial port baud rate. 
 */
Serial::Serial(const std::string& port, speed_t baud_rate) {
    fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);  // Set to non-blocking mode, otherwise it'll wait for contents in buffer
    if (fd < 0) {
        std::cerr << "Failed to open COM port." << std::endl;
        return;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Failed to get COM port attributes." << std::endl;
        return;
    }

    // We always run 8N1, so that's hard coded into the constructor.
    // May need to revisit this if we use a non-standard device.
    tty.c_cflag &= ~PARENB;                 // No parity
    tty.c_cflag &= ~CSTOPB;                 // 1 stop bit
    tty.c_cflag &= ~CSIZE;                  // Clear data bit field
    tty.c_cflag |= CS8;                     // 8 data bits
    tty.c_cflag &= ~CRTSCTS;                // No hw flow control
    tty.c_cflag |= CREAD | CLOCAL;          // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;                 // Disable canonical mode
    tty.c_lflag &= ~ECHO;                   // Disable echo
    tty.c_lflag &= ~ECHOE;                  // Disable erasure
    tty.c_lflag &= ~ECHONL;                 // Disable new-line echo
    tty.c_lflag &= ~ISIG;                   // Disable interrupt signals
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No sw flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable special handling
    tty.c_oflag &= ~OPOST;                  // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR;                  // Prevent conversion of newline to carriage return/line feed
    tty.c_cc[VTIME] = 10;                   // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Failed to update COM port settings." << std::endl;
    }
}

/**
 * Destructor for Serial object.
*/
Serial::~Serial() {
    close(fd);
}

/**
 * Writes a bytestream (raw bytes or hex) to the serial port.
 * This low-level function that does not perform any additional formatting or error checking.
 * 
 * @param data Pointer to the data buffer to be written.
 * @param size The number of bytes to write from the data buffer.
 */
void Serial::writeBytestream(const void* data, size_t size) {
    ::write(fd, data, size);
}

/**
 * Writes a string to the serial port.
 * 
 * @param str The string to be written to the serial port.
 */
void Serial::writeString(const std::string& str) {
    ssize_t bytes_written = write(fd, str.c_str(), str.size());
    if (bytes_written < 0) {
        std::cerr << "Failed to write to serial port: " << strerror(errno) << std::endl;
    }
}

/**
 * Writes ASCII data to the serial port.
 * 
 * @param asciiData The ASCII string to be written to the serial port.
 * @param length The number of bytes to write.
 */
void Serial::writeASCII(const char* asciiData, size_t length) {
    if (asciiData != nullptr && length > 0) {
        ::write(fd, asciiData, length);
    }
}

/**
 * Reads the data in the serial buffer and returns it as a string.
 * 
 * @return std::string data
*/
std::string Serial::read() {
    char buf[256];
    ssize_t n = ::read(fd, buf, sizeof(buf));
    if (n > 0) {
        return std::string(buf, n);
    }
    return "";
}
