#ifndef HTTPBUFFEREDDIRECTORY_H
#define HTTPBUFFEREDDIRECTORY_H

#include "http/HttpGlobal.h"
#include "http/HttpBufferedString.h"

namespace http {

class HttpBufferedDirectory : public HttpBufferedString {
public:
    HttpBufferedDirectory(const std::string& path, const std::string& uri);
    ~HttpBufferedDirectory();
};

}

#endif
