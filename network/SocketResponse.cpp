#include "SocketResponse.h"
#include "SocketRequest.h"

NS_CC_EXT_BEGIN

SocketResponse* SocketResponse::createResponse(SocketRequest* request)
{
    SocketResponse* response = new SocketResponse();
    if (response && response->init(request))
    {
        response->autorelease();
        return response;
    }
    
    CC_SAFE_DELETE(response);
    return NULL;
}

SocketResponse::SocketResponse():
m_request(NULL),
m_data(NULL),
m_size(0),
m_id(0)
{
    
}

SocketResponse::~SocketResponse()
{
    CC_SAFE_RELEASE(m_request);
    CC_SAFE_DELETE_ARRAY(m_data);
}

bool SocketResponse::init(SocketRequest* request)
{
    m_request = request;
    if (m_request)
    {
        m_request->retain();
    }
    return  true;
}

void SocketResponse::setData(int id, size_t size, char* data)
{
    m_id = id;
    m_size = size;
    CC_SAFE_DELETE_ARRAY(m_data);
    m_data = data;
}

NS_CC_EXT_END