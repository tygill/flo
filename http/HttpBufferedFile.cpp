#include "http/HttpBufferedFile.h"

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace http {

HttpBufferedFile::HttpBufferedFile(const std::string& path, int size) :
    file(path.c_str()),
    fileSize(size),
    headerSent(false)
{
    setStatus(200);
    if (fileSize < 0) {
        struct stat filestat;
        if (stat(path.c_str(), &filestat) == 0) {
            fileSize = filestat.st_size;
        } else {
            setStatus(404);
        }
    }
    if (!file.is_open()) {
        setStatus(404);
    }
}

HttpBufferedFile::~HttpBufferedFile() {
    if (file.is_open()) {
        file.close();
    }
}

void HttpBufferedFile::resetStream() {
    if (file.is_open()) {
        file.seekg(std::ios_base::beg);
    }
    headerSent = false;
}

bool HttpBufferedFile::streamComplete() const {
    return !file.is_open();
}

std::string HttpBufferedFile::readStream(int size) {
    if (!headerSent) {
        headerSent = true;
        std::stringstream str;
        str << fileSize;
        
        std::string buffer("Content-Length: ");
        buffer.append(str.str());
        buffer.append("\r\n\r\n");
        return buffer;
    } else if (file.is_open()) {
        std::string buffer;
        buffer.reserve(size);
        char c;
        for (int i = 0; i < size && !file.fail(); i++) {
            file.read(&c, 1);
            buffer.push_back(c);
        }
        if (file.fail()) {
            file.close();
        }
        return buffer;
    } else {
        return "";
    }
}

}
