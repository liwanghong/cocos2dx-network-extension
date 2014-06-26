#ifndef __SOCKETREQUEST_H__
#define __SOCKETREQUEST_H__

#include "cocos2d.h"
#include "ExtensionMacros.h"

NS_CC_EXT_BEGIN

#define   MESSAGE_HEAD_FIXED     ((short)0x2425)

class SocketClient;
class SocketResponse;

typedef void (CCObject::*SEL_SocketResponse)(SocketClient* client, SocketResponse* response);
#define socketresponse_selector(_SELECTOR) (cocos2d::extension::SEL_SocketResponse)(&_SELECTOR)

#define  swap_i2(i)    ((((i)&0xFF00)>>8)|(((i)&0x00FF)<<8))

struct RequestData
{
    unsigned short   message_head;
    unsigned short   size;
    unsigned short   tag;
    unsigned short   id;
    char             data[2]; //memory aligned
};

class SocketRequest: public CCObject
{
public:
    SocketRequest();
    virtual ~SocketRequest();
    
    
    static SocketRequest*  createRequest(int id, const char* data, int length);
    
    bool  init(int id, const char* data, int length);
    
    size_t    size() const
    {
        return m_size;
    }
    
    const char* data() const
    {
        return m_data;
    }
    
    int      id() const
    {
        return m_id;
    }
    
    unsigned short     tag() const
    {
        return m_tag;
    }
    
    inline void setScriptCallBack(int nHandler)
    {
        m_scirptHandler = nHandler;
    }
    
    inline int getScriptCallBack()
    {
        return m_scirptHandler;
    }
    
    inline void setResponseCallback(CCObject* pTarget, SEL_SocketResponse pSelector)
    {
        m_target = pTarget;
        m_selector = pSelector;
        
        if (m_target)
        {
            m_target->retain();
        }
    }
    
    inline CCObject* getTarget()
    {
        return m_target;
    }
    
    inline SEL_SocketResponse getSelector()
    {
        return m_selector;
    }
    
    void  invokeCallback(SocketClient* client, SocketResponse* response);
    
protected:
    size_t    m_size;
    char*     m_data;
    int       m_scirptHandler;
    int       m_id;
    unsigned short       m_tag;
    CCObject* m_target;
    SEL_SocketResponse m_selector;
};

NS_CC_EXT_END

#endif /* __SOCKETREQUEST_H__ */