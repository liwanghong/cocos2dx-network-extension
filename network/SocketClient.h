#ifndef __SOCKETCLIENT_H__
#define __SOCKETCLIENT_H__

#include "cocos2d.h"
#include "ExtensionMacros.h"
#include "SocketRequest.h"
#include <pthread.h>

NS_CC_EXT_BEGIN

class SocketClient: public CCObject
{
public:
    virtual ~SocketClient();
    
    static SocketClient* sharedSocketClient();
    static void          purgeSocketClient();
    
    bool  init();
    bool  connect(const char* ip, int port);
    virtual void update(float dt);
    
    /* This api can not invoke out cocos2dx main thread */
    bool  sendRequest(SocketRequest* request);
    
    void  getResponseData(int fd);
    
    void  registerNotifyScriptHandler(int notifyId, int hanler);
    void  registerNotifyScriptHandler(int handler);
    
    void  registerNotifyHandler(int notifyId, CCObject* target, SEL_SocketResponse selector);
    void  registerNotifyHandler(CCObject* target, SEL_SocketResponse selector);
    
protected:
    SocketClient();
    void  recvSocketData(int fd, size_t size, char* data);
    void  deinitSocket();
    
    struct ResponseData
    {
        int     id;
        int     tag;
        size_t  size;
        char*   data;
    };
    
    static SocketClient*  m_instance;
    
    int   m_socked_fd;
    
    CCDictionary*             m_responseDictionary;
    std::list<ResponseData>   m_responseData;
    pthread_mutex_t           m_responseMutex;
    CCArray*                  m_globalServiceHandler;
    CCDictionary*             m_targetServicHandler;
};

NS_CC_EXT_END


#endif /* __SOCKETCLIENT_H__ */