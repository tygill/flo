#ifndef HTTPBUFFEREDCGI_H
#define HTTPBUFFEREDCGI_H

#include "http/HttpGlobal.h"
#include "http/HttpBufferedContent.h"
#include "http/HttpRequest.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

namespace http {

class HttpBufferedCgi : public HttpBufferedContent {
public:
    HttpBufferedCgi(const std::string& path, HttpRequest* req);
    ~HttpBufferedCgi();
    
    void resetStream();
    bool streamComplete() const;
    std::string readStream(int size = 1024);
    
private:
    HttpRequest* request;
    std::string path;
    
    // store the pipes here, then read from them to return through readStream()
    int pipeTo[2];
    int pipeFrom[2];
    bool initialized;
    bool complete;
};

}

#endif
