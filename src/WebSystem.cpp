#include <signal.h>
#include <iostream>
#include <type_traits>
#include <unistd.h>
#include <functional>
#include "WebSystem.h"
#include "ThreadUtils.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

std::atomic<bool> WebSystem::m_webSocketEnabled{false};     //! Atomic boolean to check if websocket is enabled


/**
 * Constructor
 * Initializes the WebSystem with default values.
*/
WebSystem::WebSystem() :
    m_serviceThread(INVALID_PTHREAD),
    m_protocols{
        { "http", WebSystem::callbackHttp, 0, 0, 0, NULL}, 
        { "ws-protocol-text", WebSystem::callbackWsProtocolText, sizeof(int),
            WebSystem::MaxPacketByteLen, 0, NULL}, 
        { "ws-protocol-binary", WebSystem::callbackWsProtocolBinary, sizeof(int),
            WebSystem::MaxPacketByteLen, 0, NULL}, 
        { NULL, NULL, 0, 0, 0, NULL}, 
    }
{
    // Set the working directory to the directory containing the files to serve
    if (chdir("/var/www/webFiles") != 0) {
        cerr << "chdir() to /var/www/webFiles failed" << endl;
    }

    // Write buffer space for LWS
    m_writeBufferText.resize(LWS_PRE);
    m_writeBufferBinary.resize(LWS_PRE);

    m_webSocketEnabled.store(false);

}

/**
 * Destructor
 * Cleans up resources, ensures the service thread is terminated properly, and destroys the LWS context.
*/
WebSystem::~WebSystem() {
    if ( (m_serviceThread != INVALID_PTHREAD) && !m_serviceParams.exited )
    {   
        // Wait here for the thread to die
    }   

    if (m_serviceParams.context) {
        lws_context_destroy(m_serviceParams.context);
    }   
}

/**
* Initialize the service thread tied to a core
* 
* @param name The name of the application.
* @param port The port number for the binary and text websocket services.
* @param core The CPU core number to which the low priority service thread is tied.
* @param pMount The mount location for serving HTTP files.
* @param wsi Pointer to the websocket instance.
* @return int Returns 0 on success, -1 on failure.
*/
int WebSystem::initialize(string name, int port, unsigned core, const lws_http_mount* pMount) {
    m_applicationName = name;

    cout << "WebSystem: Starting initialization for " << name << " on port " << port << endl;


#ifdef TEST_MODE 
    int logs = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_HEADER | LLL_EXT | LLL_CLIENT | LLL_LATENCY | LLL_USER; // | LLL_PARSER;
#else
    int logs = 0; // Disable logs
#endif

    lws_set_log_level(logs, NULL);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.mounts = pMount;
    info.protocols = m_protocols;

    if (info.protocols != nullptr) {
        cout << "WebSystem: Protocols initialized successfully" << endl;

#ifdef TEST_MODE
        for (int i = 0; m_protocols[i].name != nullptr; ++i) {
            cout << "Protocol " << i << ": " << m_protocols[i].name << endl;
        }
#endif
    } else {
        cerr << "WebSystem: Failed to initialize protocols" << endl;
    }

    info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS;

    cout << "WebSystem: Creating lws context" << endl;
    m_serviceParams.context = lws_create_context(&info);
    if (!m_serviceParams.context) {
        cerr << "WebSystem: Failed to create lws_context" << endl;
        return -1;
    }

    cout << "WebSystem: lws context created successfully" << endl;

    cout << "WebSystem: Creating vhost" << endl;
    info.vhost_name = name.c_str();
    auto* pVhost = lws_create_vhost(m_serviceParams.context, &info);
    if (!pVhost) {
        cerr << "WebSystem: Failed to create vhost" << endl;
        return -1;
    }

    cout << "WebSystem: vhost created successfully" << endl;
    
    // Start the service thread
    m_serviceParams.exit = false;
    m_serviceParams.exited = false;

    cout << "WebSystem: Starting service thread" << endl;
    vector<unsigned> cores = { core };
    m_serviceThread = ThreadUtils::startThread(
        "WebSystem",
        serviceThread,
        &m_serviceParams,
        cores,
        false,
        false,
        sched_get_priority_min(SCHED_FIFO) + 1,
        SCHED_FIFO
    );

    if (m_serviceThread == INVALID_PTHREAD) {
        cerr << "WebSystem: Failed to start service thread" << endl;
        return -1;
    }

    cout << "WebSystem: Service thread started successfully" << endl;
    cout << "WebSystem: Initialization completed successfully" << endl;

    return 0;
}

/**
 * Service thread function
 * Handles the lifecycle of the service thread, processing websocket activity.
 * 
 * @param arg Pointer to ServiceParams_t structure containing thread and context information.
 * @return void* Always returns 0.
 */
void* WebSystem::serviceThread(void* arg) {
    ServiceParams_t* pParams = (ServiceParams_t*)(arg);
    printf("serviceThread: ServiceThread started\n");
    
    while (!pParams->exit && pParams->context) {
        printf("serviceThread: Calling lws_service\n");
        
        // Service any pending websocket activity
        int n = lws_service(pParams->context, 0);
        printf("serviceThread: lws_service returned: %d\n", n);

        if (n < 0) {
            cerr << "WebSystem::serviceThread lws_service error" << endl;
            break;
        }
        usleep(25000); // 25ms response
    }
    pParams->exited = true;
    printf("WebSystem: serviceThread exiting\n");
    return 0;
}

/**
 * Sends text data over the websocket.
 * 
 * @param str The string data to send.
 */
void WebSystem::sendTextData(const string& str) {
    lock_guard<mutex> lck(m_writeBufferTextMutex);

    if (m_webSocketEnabled.load() == false) {
        cerr << "sendTextData: WebSocket is not enabled" << endl;
        return;
    }

    if (m_webSocketEnabled.load() == true && (m_writeBufferText.size() + str.size()) > MaxPacketByteLen) {
        cerr << "sendTextData: WebSocket enabled but write buffer is too large, dumping buffer" << endl;
        cerr << "WebSystem: Current Buffer: " << m_writeBufferText.size() << "Data Buffer: " << str.size() << "Total: " << m_writeBufferText.size() + str.size() << endl;
        
        // Dump all data in the buffer
        m_writeBufferText.clear();
        m_writeBufferText.resize(LWS_PRE); // Reset buffer to initial size

        return;
    }

    // Insert new data to the end due to LWS header
    printf("sendTextData: Adding data to write buffer: %s\n", str.c_str());
    m_writeBufferText += str;

    // Service the writable callback for the text protocol
     int result = lws_callback_on_writable_all_protocol(m_serviceParams.context, &m_protocols[1]);
     if (result != 0) {
         cerr << "Failed to request writable callback, error: " << result << endl;
     } else {
         printf("sendTextData: Requested writable callback for protocol: %s\n", m_protocols[1].name);
     }

}

/**
 * Sends binary data over the websocket.
 * 
 * @param data The binary data to send as a vector of uint8_t.
 */
void WebSystem::sendBinaryData(const vector<uint8_t>& data) {
    lock_guard<mutex> lck(m_writeBufferBinaryMutex);

    // Insert new data to the end due to LWS header
    m_writeBufferBinary.insert(m_writeBufferBinary.end(), data.begin(), data.end());

    // Service the writable callback for the binary protocol
    lws_callback_on_writable_all_protocol(m_serviceParams.context, &m_protocols[2]);
}

/**
 * LWS callback for handling HTTP requests.
 * 
 * @param wsi Pointer to the websocket instance.
 * @param reason The callback reason or trigger.
 * @param user Pointer to user-defined data (unused here).
 * @param in Pointer to incoming data (unused here).
 * @param len Length of the incoming data (unused here).
 * @return int Always returns 0.
 */
int WebSystem::callbackHttp(lws* wsi, lws_callback_reasons reason, void* user, void* in, size_t len) {
    const char* file_path = "index.html";
    switch(reason) {
    case LWS_CALLBACK_HTTP:
        printf("HTTP request callback triggered\n");
        printf("Attempting to serve file: %s\n", file_path);
        if (lws_serve_http_file(wsi, file_path, "text/html", NULL, 0) < 0) {
            printf("Failed to serve %s\n", file_path);
        } else {
            printf("Successfully served %s\n", file_path);
        }
        break;
    default:
        break;
    }
    return 0;
}

/**
 * LWS callback for handling text protocol websocket communication.
 * 
 * @param wsi Pointer to the websocket instance.
 * @param reason The callback reason or trigger.
 * @param user Pointer to user-defined data (unused here).
 * @param pDataIn Pointer to incoming data.
 * @param size Size of the incoming data.
 * @return int Always returns 0.
 */
int WebSystem::callbackWsProtocolText(lws* wsi, lws_callback_reasons reason, void* user, void* pDataIn, size_t size) {
    
    // Check if wsi is null
    if (wsi == nullptr) {
        cerr << "Websocket instance is null" << endl;
        return -1; 
    }

    // Verify context association
    if (lws_get_context(wsi) == nullptr) {
        cerr << "Websocket instance is not associated with a valid context" << endl;
        return -1;
    }

    // Check protocol
    const struct lws_protocols* protocol = lws_get_protocol(wsi);
    if (protocol == nullptr || strcmp(protocol->name, "ws-protocol-text") != 0) {
        cerr << "Websocket instance is using an unexpected protocol" << endl;
        return -1; 
    }

    printf("callbackWsProtocolText triggered with reason: %d\n", reason);

    switch(reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
    {
        printf("LWS_CALLBACK_PROTOCOL_INIT triggered\n");
        break;
    }
    case LWS_CALLBACK_ESTABLISHED:
    {
        printf("LWS_CALLBACK_ESTABLISHED: Connection established for ws-protocol-text.\n");
        m_webSocketEnabled.store(true);
        printf("WebSocket readyState: OPEN\n");
        break;
    }
    case LWS_CALLBACK_CLOSED:
    {
        printf("WebSystem: Connection closed for ws-protocol-text.\n");
        m_webSocketEnabled.store(false);
        printf("WebSocket readyState: CLOSED\n");
        break;
    }   
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
    {
        printf("LWS_CALLBACK_EVENT_WAIT_CANCELLED triggered\n");
        break;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        printf("LWS_CALLBACK_SERVER_WRITEABLE triggered\n");
        lock_guard<mutex> lck(m_writeBufferTextMutex);
        size_t len = m_writeBufferText.size() - LWS_PRE;

        printf("Write buffer length: %zu\n", len);

        if(len == 0) {
            printf("No data to write\n");
            return 0;
        }

        if (len <= MaxPacketByteLen) {
            printf("Writing data to websocket\n");
            lws_write(wsi, (uint8_t*)m_writeBufferText.data() + LWS_PRE, len, LWS_WRITE_TEXT);
        } else {
            printf("WebSystem: write buffer is too large. %d\n", (int)len);
        }

        m_writeBufferText.clear();
        m_writeBufferText.resize(LWS_PRE);

        break;
    }
    case LWS_CALLBACK_RECEIVE:
    {
        printf("WebSystem: LWS_CALLBACK_RECEIVE triggered\n");
        printf("WebSystem: Received data size: %zu\n", size);

        if (size > MaxPacketByteLen) {
            printf("WebSystem: received message too large\n");
        }

        if(size == 0) {
            printf("WebSystem: No data received\n");
            return 0;
        }

        lock_guard<mutex> lck(m_readBufferTextMutex);

        uint8_t* pData = static_cast<uint8_t*>(pDataIn);
        string receivedData(pData, pData + size);
        m_readBufferText += receivedData;

        printf("Received data: %s\n", receivedData.c_str());

        try {
            m_commandData = json::parse(receivedData);  // Store the parsed JSON
            if (m_commandData.contains("command")) {
                string command = m_commandData["command"];
                auto it = m_commandCallbacks.find(command);
                if (it != m_commandCallbacks.end()) {
                    printf("Executing callback for command: %s\n", command.c_str());
                    it->second();
                }
            }
        } catch (const json::parse_error& e) {
            cerr << "Failed to parse JSON: " << e.what() << endl;
        }

#ifdef TEST_MODE
        // Echo the received message back
        {
            lock_guard<mutex> lck(m_writeBufferTextMutex);
            m_writeBufferText += receivedData;
            printf("Echoing back received message\n");
            lws_callback_on_writable(wsi);
        }
#endif
        break;
    }
    default:
        printf("Unhandled callback reason: %d\n", reason);
        break;
    }

    return 0;
}

/**
 * LWS callback for handling binary protocol websocket communication.
 * 
 * @param wsi Pointer to the websocket instance.
 * @param reason The callback reason or trigger.
 * @param user Pointer to user-defined data (unused here).
 * @param pDataIn Pointer to incoming data.
 * @param size Size of the incoming data.
 * @return int Always returns 0.
 */
int WebSystem::callbackWsProtocolBinary(lws* wsi, lws_callback_reasons reason, void* user, void* pDataIn, size_t size) {
    WebSystem* pWebSystem = static_cast<WebSystem*>(user);

    printf("callbackWsProtocolBinary triggered with reason: %d\n", reason);

    switch( reason ) {
    case LWS_CALLBACK_ESTABLISHED:
    {
        printf("LWS_CALLBACK_ESTABLISHED: Connection established for ws-protocol-binary.\n");
        m_webSocketEnabled.store(true);
        printf("WebSocket readyState: OPEN\n");
        break;
    }
    case LWS_CALLBACK_CLOSED:
    {
        printf("Connection closed for protocol: %s\n", pWebSystem->m_protocols[2].name);
        pWebSystem->m_webSocketEnabled.store(false);
        break;
    }   
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
    {
        printf("LWS_CALLBACK_EVENT_WAIT_CANCELLED triggered\n");
        break;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
        lock_guard<mutex> lck(pWebSystem->m_writeBufferBinaryMutex);
        size_t len = pWebSystem->m_writeBufferBinary.size() - LWS_PRE;

        if(len == 0)
            return 0;

        if (len <= MaxPacketByteLen)
        {
            lws_write(wsi, pWebSystem->m_writeBufferBinary.data() + LWS_PRE, len, LWS_WRITE_BINARY);
        }
        else
        {
            printf( "WebSystem: write buffer is too large. %d\n", (int)len);
        }

        pWebSystem->m_writeBufferBinary.clear();
        pWebSystem->m_writeBufferBinary.resize(LWS_PRE);

        break;
    }
    case LWS_CALLBACK_RECEIVE:
    {
        if (size > MaxPacketByteLen)
        {
            printf( "WebSystem: receive binary too large");
        }

        if(size == 0)
            return 0;

        lock_guard<mutex> lck(pWebSystem->m_readBufferBinaryMutex);

        uint8_t* pData = static_cast<uint8_t*>(pDataIn);
        pWebSystem->m_readBufferBinary.insert(pWebSystem->m_readBufferBinary.end(), pData, pData+size);

        break;
    }
    default:
        break;
    }
    
    return 0;
}

WebSystem::ServiceParams_t& WebSystem::getServiceParams() {
    return m_serviceParams;
}
