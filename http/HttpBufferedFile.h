#ifndef HTTPBUFFEREDFILE_H
#define HTTPBUFFEREDFILE_H

#include <fstream>

#include "http/HttpGlobal.h"
#include "http/HttpBufferedContent.h"

namespace http {

class HttpBufferedFile : public HttpBufferedContent {
public:
    HttpBufferedFile(const std::string& path, int size = -1);
    ~HttpBufferedFile();

    void resetStream();
    bool streamComplete() const;
    std::string readStream(int size = 1024);
    
private:
    std::ifstream file;
    int fileSize;
    bool headerSent;
};

}

#endif
