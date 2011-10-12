#ifndef HTTPBUFFEREDSTRING_H
#define HTTPBUFFEREDSTRING_H

#include "http/HttpGlobal.h"
#include "http/HttpBufferedContent.h"

namespace http {

class HttpBufferedString : public HttpBufferedContent {
public:
    HttpBufferedString(const std::string& str);
    ~HttpBufferedString();
    
    void resetStream();
    bool streamComplete() const;
    std::string readStream(int size = 1024);
    
protected:
    void setBuffer(const std::string& str);
private:
    std::string buffer;
    unsigned int index;
};

}

#endif
