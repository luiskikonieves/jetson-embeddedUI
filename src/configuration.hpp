/**
* Class for reading and writing configuration files stored in Mission Computer EEPROM.  
*/
#ifndef _CONFIGURATION_HPP
#define _CONFIGURATION_HPP

#include <map>
#include <string>
#include <nlohmann/json.hpp>
#include <vector>

#define FACTORY_FILE_PATH "configuration/factorySettings.json"

class Settings {
public:
    struct Server {
        int port;
    }; // Server

    struct IO {
        uint8_t pinNumber;
        std::string port;
        std::string pinFunction;
        std::string pinName;
        std::string direction;
        std::vector<uint16_t> setPoints;
        size_t initialValue;
        bool isEnabled;
    }; // IO

    Settings(const std::string& filePath);

    void saveSettings(const std::string& filePath);

    std::string findIOKeyByPinName(const std::string& pinName) const;

    Server serverSettings;
    std::map<std::string, IO> ioSettings;

private:

};

#endif // _CONFIGURATION_HPP