#include "http/HttpGlobal.h"

namespace http {

std::string versionToString(HttpVersion version) {
    switch (version) {
    case HTTP_1_0:
        return "HTTP/1.0";
    break;
    case HTTP_1_1:
        return "HTTP/1.1";
    break;
    case HTTP_UNKNOWN_VERSION:
    default:
        return "HTTP/???";
    break;
    }
}

std::string requestTypeToString(HttpRequestType type) {
    switch (type) {
    case HTTP_GET:
        return "GET";
    break;
    case HTTP_UNKNOWN_REQUEST:
    default:
        return "UNKNOWN";
    break;
    }
}

std::string trim(const std::string& str) {
    // Not very well optimized I know, but still
    std::string copy = str;
    while (copy.length() > 0 && isspace(copy.at(0))) {
        copy.erase(0, 1);
    }
    while (copy.length() > 0 && isspace(copy.at(copy.length() - 1))) {
        copy.erase(copy.length() - 1);
    }
    return copy;
}

}
