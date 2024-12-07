/**
* Low-level server functionality. Initializes server and thread, handles callbacks,
* sends and receives data via the websocket. All of this is handled in a thread
* separate from the main application thread.
* 
*/

#pragma once

#include <libwebsockets.h>
#include <pthread.h>
#include <mutex>
#include <vector>
#include <string.h>
#include <functional> 
#include <atomic>
#include <unordered_map>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class WebSystem
{
protected:
    //! Child provides a service routine to run in the main loop in the application
    virtual void service() = 0;

    int initialize(std::string name, int port, unsigned core, const lws_http_mount* mount);

    //! RAII process for the service routine to get string commands.
    //! For use in the service routine to read the commands from the ws.
    //! Create object in service() routine to lock/read/unlock command buffer.
    struct Reader {
        Reader() {
            m_readBufferTextMutex.lock();
        }
        ~Reader() {
            m_readBufferText.clear();
            m_readBufferTextMutex.unlock();
        }
        const std::string& Commands(void) {
            return m_readBufferText;
        }
    };  

    //! RAII process for the service routine to get binary data.
    //! For use in the service routine to read the binary data from the ws.
    //! Create object in service() routine to lock/read/unlock binary buffer.
    struct BinaryReader {
        BinaryReader() {
            m_readBufferBinaryMutex.lock();
        }
        ~BinaryReader() {
            m_readBufferBinary.clear();
            m_readBufferBinaryMutex.unlock();
        }
        const std::vector<uint8_t>& Data(void) {
            return m_readBufferBinary;
        }
    };

    static int callbackHttp(lws *wsi, lws_callback_reasons reason,
        void* user, void* data, size_t dataLen);

    static int callbackWsProtocolText(lws *wsi, lws_callback_reasons reason,
        void* user, void* data, size_t dataLen);

    static int callbackWsProtocolBinary(lws *wsi, lws_callback_reasons reason,
        void* user, void* data, size_t dataLen);

    //! Stream binary data to web socket
    //! Allows for any data type to be streamed to the websocket
    template <class T>
    void sendData(const std::vector<T>& data)
    {
        size_t len = data.size();
        size_t dataSize = sizeof(T);
        uint8_t* pData = (uint8_t*)data.data();

        std::vector<uint8_t> rawData(pData, pData + len * dataSize);
        sendBinaryData(rawData);
    }

    void sendBinaryData(const std::vector<uint8_t>& data);
    void sendTextData(const std::string& str);

    static const size_t            MaxPacketByteLen = 200000;      //! Max size of packet supported
    static std::atomic<bool>       m_webSocketEnabled;             //! Atomic boolean to check if websocket is enabled

    // Generic command callback map
    inline static std::unordered_map<std::string, std::function<void()>> m_commandCallbacks;

    static void registerCommandCallback(const std::string& command, std::function<void()> callback) {
        m_commandCallbacks[command] = std::move(callback);
    }

    static void clearCommandCallbacks() {
        m_commandCallbacks.clear();
    }

    void setCommandCallback(const std::string& command, std::function<void()> callback) {
        m_commandCallbacks[command] = callback;
    }

    inline static json m_commandData;
    const json& getCommandData() const { return m_commandData; }

private:

    //! Service thread that will check for lws services
    //! \params void* args for the ServiceParams_t
    static void* serviceThread(void* arg);
    struct ServiceParams_t
    {
        volatile bool exit = false;                         //! Signal thread to exit
        volatile bool exited = false;                       //! Thread indicates it has exited
        lws_context*  context = nullptr;                    //! LWS context
    };

    ServiceParams_t         m_serviceParams;                //! Parameters for service thread
    pthread_t               m_serviceThread;                //! Service thread to services the lws
    const lws_protocols     m_protocols[4];                 //! Protocols supported

    inline static std::mutex   m_readBufferTextMutex;       //! Data mutex for incoming requests
    inline static std::string  m_readBufferText;            //! Text input buffer
    inline static std::mutex   m_writeBufferTextMutex;      //! Data mutex for outgoing requests
    inline static std::string  m_writeBufferText;           //! Write buffer for lws callback text protocol

    inline static std::mutex   m_readBufferBinaryMutex;     //! Read binary buffer mutex
    inline static std::vector<uint8_t> m_readBufferBinary;  //! Read binary buffer
    inline static std::mutex   m_writeBufferBinaryMutex;    //! Data mutex for outgoing binary requests
    inline static std::vector<uint8_t> m_writeBufferBinary; //! Write buffer for lws callback binary protocol

    std::string m_applicationName;                          //! Name of the application
    

public:
    WebSystem();

    virtual ~WebSystem();

    lws_context* contextPtr() { return m_serviceParams.context; }
    
    ServiceParams_t& getServiceParams();
};

