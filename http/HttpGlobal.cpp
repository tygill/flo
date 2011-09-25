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

std::string statusCodeToString(int status) {
    // Yes, this could use a map for quicker lookups, but it can use switch for quicker
    // coding, which for school assignments that will be effectively abandoned, I'm OK with
    // that. If I ever continue this, this will change.
    switch (status) {
    case 200:
        return "OK";
    break;
    case 301:
        return "Moved Permanently";
    break;
    case 302:
        return "Found";
    break;
    case 303:
        return "See Other";
    break;
    case 304:
        return "Not Modified";
    break;
    case 307:
        return "Temporary Redirect";
    break;
    case 400:
        return "Bad Request";
    break;
    case 401:
        return "Unauthorized";
    break;
    case 403:
        return "Forbidden";
    break;
    case 404:
        return "Not Found";
    break;
    case 405:
        return "Method Not Allowed";
    break;
    case 418:
        return "I'm a teapot";
    break;
    case 500:
        return "Internal Server Error";
    break;
    case 501:
        return "Not Implemented";
    break;
    case 503:
        return "Service Unavailable";
    break;
    default:
        return "Unknown";
    break;
    }
}

std::string mimeType(const std::string& extension) {
    // Yes, this could use a map for quicker lookups, but it can use if/else for quicker
    // coding, which for school assignments that will be effectively abandoned, I'm OK with
    // that. If I ever continue this, this will change.
    if (extension.compare("html") == 0 || extension.compare("php") == 0) {
        return "text/html";
    } else if (extension.compare("txt") == 0) {
        return "text/plain";
    } else if (extension.compare("gif") == 0) {
        return "image/gif";
    } else if (extension.compare("jpg") == 0 || extension.compare("jpeg") == 0) {
        return "image/jpeg";
    } else {
        return "text/plain";
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
