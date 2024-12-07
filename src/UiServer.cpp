#include "UiServer.h"
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <cmath>

using namespace std;
using json = nlohmann::json;

// Mount path to the html/js/css files
const char* UiServer::MOUNT_PATH = "var/www/webFiles";
const char* UiServer::PASSWORD_PATH = "var/www/webFiles/.ba-passwords";
const char* UiServer::PASSWORD_MA_PATH = "var/www/webFiles/.ba-ma-passwords";

static const struct lws_protocol_vhost_options nvo_mime_ts = {
    NULL,
    NULL,
    ".ts",
    "video/mp2t"
};

static const struct lws_protocol_vhost_options nvo_mime_m3u8 = {
    &nvo_mime_ts,
    NULL,
    ".m3u8",
    "application/x-mpegURL"
};

static const struct lws_protocol_vhost_options nvo_mime_mp4 = {
    &nvo_mime_m3u8,
    NULL,
    ".mp4",
    "video/mp4"
};

static const struct lws_protocol_vhost_options nvo_mime_wasm = {
    &nvo_mime_mp4,
    NULL,
    ".wasm",
    "application/wasm"
};

UiServer::UiServer() :
    WebSystem(),
    context(nullptr),
    m_mount{
        &m_superMount, "/", MOUNT_PATH, "index.html", 
        NULL, &nvo_mime_wasm, 
        0, 0, 
        0, 0, 0, 
        LWSMPRO_FILE, 1,
    },
    m_superMount{
        &m_manufacturingMount, "/superuser", MOUNT_PATH, "integration.html", 
        NULL, &nvo_mime_wasm,
        0, 0, 
        0, 0, 0, 
        LWSMPRO_FILE, 10,
    },
    m_manufacturingMount{
        NULL, "/manufacturing", MOUNT_PATH, "manufacturing.html",
        NULL, &nvo_mime_wasm,
        0, 0, 
        0, 0, 0, 
        LWSMPRO_FILE, 14,
    }
{
    std::cout << "UiServer constructor: MOUNT_PATH = " << MOUNT_PATH << std::endl;
    
    // Clear any existing callbacks when creating a new UiServer instance
    clearCommandCallbacks();
}

UiServer::~UiServer() {
    if (context) {
        lws_context_destroy(context);
    }
}

/**
 * @brief Initializes the UiServer.
 * 
 * @param port Port number
 * @return true if initialization is successful, false otherwise.
 */
bool UiServer::initialize(int port) {
    std::cout << "UiServer::initialize: Starting initialization on port " << port << std::endl;
    
    // Register command callbacks before initializing WebSystem
    registerCommandCallbacks();
    
    int result = WebSystem::initialize("webapp", port, 0, &m_mount);
    if (result != 0) {
        std::cerr << "UiServer::initialize: WebSystem::initialize failed with error code " << result << std::endl;
        return false;
    }
    std::cout << "UiServer::initialize: Initialization completed successfully" << std::endl;
    return true;
}

/**
 * @brief Registers command callbacks for the UiServer.
 * 
 * This method sets up the command callbacks for handling user-generated commands.
 */
void UiServer::registerCommandCallbacks() {
    setCommandCallback("pwm-control", [this]() {
        std::cout << "PWM control command received" << std::endl;
        const auto& data = this->getCommandData();
        if (m_pwmControlCallback && data.contains("index")) {
            size_t index = data["index"].get<std::size_t>();
            std::cout << "Setting PWM to position " << index << std::endl;
            m_pwmControlCallback(index);
        }
    });
}

/**
 * @brief Services and processes outgoing data.
 */
void UiServer::service() {
    std::cout << "UiServer::service() called" << std::endl;

    Reader commandReader;
    const auto& command = commandReader.Commands();
}

/**
 * @brief Starts a process to handle HLS streaming using ffmpeg.
 */
void UiServer::startProcess() {
    // example starting a hls process
    std::string cmd = std::string("ffmpeg -hide_banner -loglevel quiet -i udp://192.168.10.10:1234 ") +
         "-vcodec copy -f hls -hls_segment_type mpegts -hls_time 0.5 -hls_wrap 10 -hls_list_size 10 /var/www/master.m3u8";
    system(cmd.c_str());

    m_processStarted = true;
}

/**
 * @brief Stops the HLS streaming process by killing the ffmpeg process.
 */
void UiServer::stopProcess() {
    // Get the pid of ffmpeg process and kill it
    char line[10];
    FILE *cmd = popen("pidof ffmpeg", "r");

    fgets(line, 10, cmd);
    pid_t pid = strtoul(line, NULL, 10);

    pclose(cmd);

    // Kill the ffmpeg process
    kill(pid, 9);
    m_processStarted = false;
}

/**
 * @brief Processes incoming binary data.
 */
void UiServer::processBinaryData() {
    // RAII, grabs binary data, clears buffer after use
    BinaryReader binaryReader;
    const auto& binaryData = binaryReader.Data();

    // File transfers can be chunked. Find the sync header,
    // mark the inProgress and receive the rest of the data.
    if(binaryData.size() > 0) {
        // Handle binary data here...
    }
}

/**
 * @brief Sends an example binary data buffer.
 */
void UiServer::sendBinaryExample() {
    // Example send data
    static std::vector<int16_t> dataBuffer;
    if(dataBuffer.capacity() == 0) dataBuffer.reserve(0);
    sendData(dataBuffer);
    dataBuffer.clear();
}

