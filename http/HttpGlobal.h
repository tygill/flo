#ifndef HTTPGLOBAL_H
#define HTTPGLOBAL_H

#include <string>
#include <cstdlib>

#define SOCKET_ERROR -1
#define MAX_THREAD_COUNT 25
#define THREAD_TIMEOUT 5

namespace http {

enum HttpVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_UNKNOWN_VERSION
};

std::string versionToString(HttpVersion version);

enum HttpRequestType {
    HTTP_GET,
    HTTP_POST,
    HTTP_UNKNOWN_REQUEST
};

std::string requestTypeToString(HttpRequestType type);

std::string statusCodeToString(int status);

std::string mimeType(const std::string& extension);

std::string trim(const std::string& string);

}

#endif
