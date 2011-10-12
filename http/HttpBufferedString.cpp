#include "http/HttpBufferedString.h"

#include <sstream>

namespace http {

HttpBufferedString::HttpBufferedString(const std::string& str) :
    index(0)
{
    std::string prefix("Content-Length: ");
    std::stringstream len;
    len << str.size();
    prefix.append(len.str());
    prefix.append("\r\n\r\n");
    
    buffer = prefix;
    buffer.append(str);
}

HttpBufferedString::~HttpBufferedString() {
}

void HttpBufferedString::resetStream() {
    index = 0;
}

bool HttpBufferedString::streamComplete() const {
    return index >= buffer.size();
}

std::string HttpBufferedString::readStream(int size) {
    std::string ret;
    ret = buffer.substr(index, size);
    index += ret.size();
    return ret;
}

void HttpBufferedString::setBuffer(const std::string& str) {
    buffer = str;
    resetStream();
}

}
