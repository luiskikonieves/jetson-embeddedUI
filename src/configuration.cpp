#include "configuration.hpp"
#include <fstream>
#include <iostream>
#include <filesystem> 

using json = nlohmann::json;

Settings::Settings(const std::string& filePath) {
    // Check if the file exists and restore it if it does not
    if (!std::filesystem::exists(filePath)) {
        std::filesystem::copy(FACTORY_FILE_PATH, filePath, std::filesystem::copy_options::overwrite_existing);
    }

    std::ifstream i(filePath);
    if (!i.is_open()) {
        throw std::runtime_error("Unable to open file: " + filePath);
    }
    json j;
    i >> j;

    serverSettings.port = j["Server"]["port"].get<int>();

    // Parse IO
    for (auto& el : j["IO"].items()) {
        IO io;
        io.pinNumber = el.value()["pinNumber"].get<int>();
        io.port = el.value()["port"];
        io.pinFunction = el.value()["pinFunction"];
        io.pinName = el.value()["pinName"];
        io.direction = el.value()["direction"];
        io.isEnabled = el.value()["isEnabled"].get<bool>();
        io.setPoints = el.value()["setPoints"].get<std::vector<uint16_t>>();
        
        io.initialValue = el.value()["initialValue"].get<size_t>();
        if (io.initialValue >= io.setPoints.size()) {
            io.initialValue = 0;
        }
        
        ioSettings[el.key()] = io;
    }
}

void Settings::saveSettings(const std::string& filePath) {
    json j;

    // Server
    j["Server"]["port"] = serverSettings.port; 

    // IO
    for (const auto& ioPair : ioSettings) {
        const auto& key = ioPair.first;
        const auto& io = ioPair.second;
        j["IO"][key]["pinNumber"] = io.pinNumber;
        j["IO"][key]["port"] = io.port;
        j["IO"][key]["pinFunction"] = io.pinFunction;
        j["IO"][key]["pinName"] = io.pinName;
        j["IO"][key]["direction"] = io.direction;
        j["IO"][key]["isEnabled"] = io.isEnabled;
        j["IO"][key]["setPoints"] = io.setPoints;
        j["IO"][key]["initialValue"] = io.initialValue;
    }

    // Write to file
    std::ofstream o(filePath);
    o << std::setw(4) << j << std::endl;
}

/**
 * Returns the key for a GPIO pinName as defined in the configuration files.
 * 
 * @param pinName String describing the pinName contained in userSettings.
 * @return String with the IO pin key
*/
std::string Settings::findIOKeyByPinName(const std::string& pinName) const {
    for (const auto& pair : ioSettings) {
        if (pair.second.pinName == pinName) {
            return pair.first;
        }
    }
    throw std::runtime_error("Pin name '" + pinName + "' not found in ioSettings.");
}
