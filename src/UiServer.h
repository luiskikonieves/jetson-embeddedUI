/**
* Sever which serves application data via a websocket. This layer
* abstracts the low level websocket functions.
*/

#ifndef UISERVER_H
#define UISERVER_H

#include <libwebsockets.h>
#include <functional> 
#include "WebSystem.h"

class UiServer : public WebSystem { 
public:

    
    UiServer(); 
    ~UiServer(); 

    bool initialize(int port);
    void service() override;

    void setPwmControlCallback(std::function<void(size_t)> callback) {
        m_pwmControlCallback = callback;
    }

private:
    struct lws_context *context;
    struct lws_protocols protocol;
    struct lws_context_creation_info info; 

    static const char* LOG_FILE_PATH;
    static const char* MOUNT_PATH;
    static const char* PASSWORD_PATH;
    static const char* PASSWORD_MA_PATH;
    bool m_processStarted = false;

    lws_http_mount m_mount;                   //! Mount location for the web files
    lws_http_mount m_superMount;              //! Super mount location
    lws_http_mount m_manufacturingMount;      //! Super mount location


    void processBinaryData();
    void sendBinaryExample();
    void startProcess();
    void stopProcess();
    void registerCommandCallbacks();

    std::function<void(size_t)> m_pwmControlCallback;
};

#endif //UISERVER_H

