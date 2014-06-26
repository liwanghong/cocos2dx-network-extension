#include "SocketClient.h"
#include <pthread.h>
#if CC_TARGET_PLATFORM != CC_PLATFORM_WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "libev/ev.h"
#else 
# include <io.h>
# include <winsock2.h>
# include <windows.h>
#include "libev/evwrap.h"
#endif

#include "SocketResponse.h"

NS_CC_EXT_BEGIN

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#define  EV_TYPE   EVBACKEND_EPOLL
#elif CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#define  EV_TYPE   EVBACKEND_KQUEUE
#elif CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
#define  EV_TYPE   EVBACKEND_SELECT
#endif

SocketClient* SocketClient::m_instance = NULL;

static void*  recieve_thread_main(void* socket);
static void   socket_recv_callback(struct ev_loop *reactor, ev_io *w, int events);

void*  recieve_thread_main(void* socket)
{
    int sokcet_fd = (int)socket;
#ifdef WIN32 
	sokcet_fd = _open_osfhandle(sokcet_fd, 0);
#endif
    struct ev_loop *loop = ev_loop_new(EV_TYPE);
    ev_io watcher;
    ev_io_init(&watcher, socket_recv_callback, sokcet_fd, EV_READ);
    ev_io_start(loop, &watcher);
    ev_run(loop,0);
    ev_loop_destroy(loop);
    return NULL;
}

void   socket_recv_callback(struct ev_loop *reactor, ev_io *w, int events)
{
    // read error
    if(EV_ERROR & events)
    {
        return;
    }
    
    SocketClient* client = SocketClient::sharedSocketClient();
    client->getResponseData(w->fd);
}

SocketClient*  SocketClient::sharedSocketClient()
{
    if (m_instance)
    {
        return m_instance;
    }
    
    m_instance = new SocketClient();
    if (!m_instance)
    {
        return NULL;
    }
    
    if (!m_instance->init())
    {
        delete m_instance;
        return NULL;
    }
    
    return m_instance;
}

void SocketClient::purgeSocketClient()
{
    if (m_instance)
    {
        delete m_instance;
        m_instance = NULL;
    }
}

SocketClient::SocketClient()
:m_responseDictionary(NULL),
m_socked_fd(-1),
m_globalServiceHandler(NULL),
m_targetServicHandler(NULL)
{
    
}

SocketClient::~SocketClient()
{
    deinitSocket();
    
    CC_SAFE_DELETE(m_responseDictionary);
    
    std::list<ResponseData>::iterator iter = m_responseData.begin();
    while (iter != m_responseData.end())
    {
        char* data = iter->data;
        CC_SAFE_DELETE_ARRAY(data);
        iter++;
    }
    m_responseData.clear();
    
    CC_SAFE_DELETE(m_globalServiceHandler);
    CC_SAFE_DELETE(m_targetServicHandler);
}

bool SocketClient::init()
{
#ifdef WIN32 
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 0);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (0 != err)
	{
		return false ;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) 
	{
		WSACleanup();
		return false ;
	}
#endif
    m_socked_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifndef WIN32
    fcntl(m_socked_fd,F_SETFL,fcntl(m_socked_fd,F_GETFL) & (~O_NONBLOCK));
#endif
	
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr)); 
    client_addr.sin_family = AF_INET;    
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    client_addr.sin_port = htons(0);   
    if( bind(m_socked_fd,(struct sockaddr*)&client_addr,sizeof(client_addr)))
    {
        return false;
    }

    m_responseDictionary = new CCDictionary();
    
    m_globalServiceHandler = CCArray::create();
    m_globalServiceHandler->retain();
    
    m_targetServicHandler = new CCDictionary();
    
    pthread_mutex_init(&m_responseMutex, NULL);
    
    CCDirector::sharedDirector()->getScheduler()->scheduleUpdateForTarget(this, 0, false);
    return true;
}

bool SocketClient::connect(const char* ip, int port)
{
    if(m_socked_fd == -1)
    {
        return false;
    }
    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    
    //sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
#ifndef WIN32
    inet_pton(AF_INET, ip, &(sin.sin_addr));
#else 
	sin.sin_addr.S_un.S_addr = inet_addr(ip);
#endif
    int ret = ::connect(m_socked_fd, (struct sockaddr*)(&sin), sizeof(sin));
    
    if (ret != 0)
    {
        CCLOG("connect with error:%d", errno);
        deinitSocket();
        return false;
    }
#ifndef WIN32
    fcntl(m_socked_fd,F_SETFL,fcntl(m_socked_fd,F_GETFL) | O_NONBLOCK);
#endif
    pthread_t thread;
    ret = pthread_create(&thread, NULL, recieve_thread_main, (void*)m_socked_fd);
    if (ret != 0)
    {
        deinitSocket();
        return false;
    }
    
    return true;
}

bool SocketClient::sendRequest(SocketRequest* request)
{
    CCLog("Send request");
    if (!m_responseDictionary)
    {
        return false;
    }
    
    unsigned short tag = request->tag();
    CCObject* old = m_responseDictionary->objectForKey(tag);
    if (old)
    {
        return NULL;
    }
    
    SocketResponse* response = SocketResponse::createResponse(request);
    
    int ret = send(m_socked_fd, request->data(), request->size(), 0);
    if (ret < -1)
    {
        return false;
    }
    
    m_responseDictionary->setObject(response, tag);
    return true;
}

void SocketClient::getResponseData(int fd)
{
    RequestData  requestData;
    memset(&requestData, 0, sizeof(RequestData));
#ifdef WIN32
	fd = _get_osfhandle(fd);
#endif
    
    recvSocketData(fd, sizeof(RequestData) - 2, (char*)(&requestData));
    requestData.message_head = swap_i2(requestData.message_head);
    requestData.size = swap_i2(requestData.size);
    requestData.id = swap_i2(requestData.id);
    requestData.tag = swap_i2(requestData.tag);
    
    if (requestData.size < 6)
    {
        return;
    }

    char* data = new char[requestData.size - 5];
    memset(data, 0, requestData.size - 5);
    
    recvSocketData(fd, requestData.size - 6, data);
    
    pthread_mutex_lock(&m_responseMutex);
    
    int id = requestData.id;
    ResponseData responseData;
    memset(&responseData, 0, sizeof(responseData));
    responseData.id = id;
    responseData.tag = requestData.tag;
    responseData.size = requestData.size - 6;
    responseData.data = data;
    m_responseData.push_back(responseData);
    
    pthread_mutex_unlock(&m_responseMutex);
}

void SocketClient::recvSocketData(int fd, size_t size, char* data)
{
    size_t offset = 0;
    while (offset < size)
    {
        size_t recv_size = recv(fd, data + offset, size - offset, MSG_WAITALL);
        if (recv_size == -1)
        {
#ifndef WIN32
            if (errno == EAGAIN)
            {
                continue;
            }
#endif
            return;
        }
        offset += recv_size;
    }
}

void SocketClient::deinitSocket()
{
    if (m_socked_fd != -1)
    {
#ifndef WIN32
        ::close(m_socked_fd);
#else
		closesocket(m_socked_fd);
		WSACleanup();
#endif
    }
    m_socked_fd = -1;
}

void SocketClient::update(float dt)
{
    CCArray  doneResponse;
    int error = pthread_mutex_trylock(&m_responseMutex);
    if (error == 0)
    {
        doneResponse.initWithCapacity(m_responseData.size());
        std::list<ResponseData>::iterator iter = m_responseData.begin();
        while (iter != m_responseData.end())
        {
            int id = iter->id;
            
            CCObject* responseObject = m_responseDictionary->objectForKey(iter->tag);
            if (responseObject == NULL)
            {
                SocketResponse*  response = SocketResponse::createResponse(NULL);
                response->setData(iter->id, iter->size, iter->data);
                doneResponse.addObject(response);
            }
            else
            {
                SocketResponse* response = (SocketResponse*)responseObject;
                response->setData(iter->id, iter->size, iter->data);
                doneResponse.addObject(responseObject);
                m_responseDictionary->removeObjectForKey(iter->tag);
            }
            iter++;
        }
        m_responseData.clear();
        pthread_mutex_unlock(&m_responseMutex);
    }
    
    for (int i = 0; i < doneResponse.count(); i++)
    {
        SocketResponse* response = (SocketResponse*)doneResponse.objectAtIndex(i);
        SocketRequest* request = response->request();
        
        if (request)
        {
            request->invokeCallback(this, response);
        }
        else
        {
            CCArray* targetHandlers = (CCArray*)m_targetServicHandler->objectForKey(response->id());
            if (targetHandlers)
            {
                for (int i = 0; i < targetHandlers->count(); i++)
                {
                    SocketRequest* request = (SocketRequest*)targetHandlers->objectAtIndex(i);
                    request->invokeCallback(this, response);
                }
            }
            
            for (int i = 0; i < m_globalServiceHandler->count(); i++)
            {
                SocketRequest* request = (SocketRequest*)m_globalServiceHandler->objectAtIndex(i);
                request->invokeCallback(this, response);
            }
        }
    }
}

void SocketClient::registerNotifyScriptHandler(int notifyId, int handler)
{
    SocketRequest* socketRequest = SocketRequest::createRequest(notifyId, "", 0);
    socketRequest->setScriptCallBack(handler);
    
    CCObject* handlerObject = m_targetServicHandler->objectForKey(notifyId);
    if (handlerObject == NULL)
    {
        CCArray*  notifyHandlers = CCArray::create();
        notifyHandlers->addObject(socketRequest);
        
        m_targetServicHandler->setObject(notifyHandlers, notifyId);
    }
    else
    {
        CCArray* notifyHandlers = (CCArray*)handlerObject;
        notifyHandlers->addObject(socketRequest);
    }
}

void SocketClient::registerNotifyScriptHandler(int handler)
{
    SocketRequest* socketRequest = SocketRequest::createRequest(0, "", 0);
    socketRequest->setScriptCallBack(handler);
    m_globalServiceHandler->addObject(socketRequest);
}

void SocketClient::registerNotifyHandler(int notifyId, CCObject* target, SEL_SocketResponse selector)
{
    SocketRequest* socketRequest = SocketRequest::createRequest(notifyId, "", 0);
    socketRequest->setResponseCallback(target, selector);
    
    CCObject* handlerObject = m_targetServicHandler->objectForKey(notifyId);
    if (handlerObject == NULL)
    {
        CCArray*  notifyHandlers = CCArray::create();
        notifyHandlers->addObject(socketRequest);
        
        m_targetServicHandler->setObject(notifyHandlers, notifyId);
    }
    else
    {
        CCArray* notifyHandlers = (CCArray*)handlerObject;
        notifyHandlers->addObject(socketRequest);
    }
}

void SocketClient::registerNotifyHandler(CCObject* target, SEL_SocketResponse selector)
{
    SocketRequest* socketRequest = SocketRequest::createRequest(0, "", 0);
    socketRequest->setResponseCallback(target, selector);
    m_globalServiceHandler->addObject(socketRequest);
}

NS_CC_EXT_END
