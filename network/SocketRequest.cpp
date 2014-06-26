#include "SocketRequest.h"
#include "SocketClient.h"
#include "SocketResponse.h"

NS_CC_EXT_BEGIN

SocketRequest* SocketRequest::createRequest(int id, const char* data, int length)
{
    SocketRequest* request = new SocketRequest();
    if (request && request->init(id, data, length))
    {
        request->autorelease();
        return request;
    }
    
    CC_SAFE_DELETE(request);
    return NULL;
}

SocketRequest::SocketRequest()
:m_size(0),
m_data(NULL),
m_scirptHandler(0),
m_id(0),
m_target(NULL),
m_selector(NULL)
{
    
}

SocketRequest::~SocketRequest()
{
    CC_SAFE_DELETE(m_data);
    CC_SAFE_RELEASE(m_target);
}

bool SocketRequest::init(int id, const char* data, int length)
{
    static short tag = 0;
    m_size = sizeof(RequestData) + length - 2;
    
    RequestData* b_data = (RequestData*)malloc(m_size);
    
    if (!b_data)
    {
        return false;
    }

    m_id = id;
    m_tag = ++tag;
    
    b_data->message_head = swap_i2(MESSAGE_HEAD_FIXED);
    b_data->id = swap_i2((short)id);
    b_data->tag = swap_i2((short)m_tag);
    b_data->size = swap_i2(m_size - 2);
    
    memcpy(b_data->data, data, sizeof(char) * length);
    m_data = (char*)b_data;
    
    return true;
}

void SocketRequest::invokeCallback(SocketClient* client, SocketResponse* response)
{
    if (m_target && m_selector)
    {
        (m_target->*m_selector)(client, response);
    }
    
    if (m_scirptHandler != 0)
    {
        CCScriptEngineManager::sharedManager()->getScriptEngine()->executeHttpRequestEvent(m_scirptHandler, client, response);
    }

}


NS_CC_EXT_END