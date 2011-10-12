#ifndef HTTPBUFFEREDCONTENT_H
#define HTTPBUFFEREDCONTENT_H

#include "http/HttpGlobal.h"

namespace http {

class HttpBufferedContent {
public:
    HttpBufferedContent();
    virtual ~HttpBufferedContent();
    
    int status() const;
    virtual void resetStream() = 0;
    virtual bool streamComplete() const = 0;
    virtual std::string readStream(int size = 1024) = 0;
    
protected:
    void setStatus(int stats);
private:
    int code;
};

}

#endif
