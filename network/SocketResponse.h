#ifndef __SOCKETRESPONSE_H__
#define __SOCKETRESPONSE_H__

#include "cocos2d.h"
#include "ExtensionMacros.h"

NS_CC_EXT_BEGIN

class SocketRequest;

class SocketResponse: public CCObject
{
public:
    SocketResponse();
    virtual ~SocketResponse();
    
    static SocketResponse*   createResponse(SocketRequest* request);
    
    bool  init(SocketRequest* request);
    
    SocketRequest*  request()
    {
        return m_request;
    }
    
    size_t size() const
    {
        return m_size;
    }
    
    const void* data() const
    {
        return m_data;
    }
    
    int id() const
    {
        return m_id;
    }
    
    void   setData(int id, size_t size, char* data);
    
protected:
    SocketRequest*  m_request;
    char*           m_data;
    size_t          m_size;
    int             m_id;
};

NS_CC_EXT_END

#endif /* __SOCKETRESPONSE_H__ */