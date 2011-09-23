#include "http/HttpRequest.h"
#include "http/HttpGlobal.h"

#include <string>
#include <iostream>
#include <cstdio>
#include <cctype>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>

namespace http {

HttpRequest::HttpRequest() :
    type(HTTP_GET),
    uri("/"),
    version(HTTP_1_1)
{
    // Nothing else to construct
}

HttpRequest::HttpRequest(HttpTokenizer& tokenizer) :
    type(HTTP_GET),
    uri("/"),
    version(HTTP_1_1)
{
    // Initialize via tokenizer
    bool advanceTokenizer = true;
    while (tokenizer.hasToken()) {
        std::string name;
        switch (tokenizer.tokenType()) {
        case HttpTokenizer::HttpStatus:
            // We should not be seeing this particular token, as we are trying to
            // parse a request
        break;
        case HttpTokenizer::HttpRequest:
            // Parse out the various pieces of the request line.
            // This is built into another function though.
            setRequest(tokenizer.tokenValue());
        break;
        case HttpTokenizer::HeaderField:
            // We're going to grab the next token, and make sure it's a
            // HeaderValue token, so store the HeaderField
            name = trim(tokenizer.tokenValue());
            tokenizer.next();
            if (tokenizer.hasToken() && tokenizer.tokenType() == HttpTokenizer::HeaderValue) {
                addHeader(name, tokenizer.tokenValue());
                //std::cout << "Header:" << std::endl;
                //std::cout << name << ": " << tokenizer.tokenValue() << std::endl;
                // Set the content reserve size if we found the Content-Length header
                if (name.compare("Content-Length") == 0) {
                    tokenizer.setContentReserveSize(atoi(tokenizer.tokenValue().c_str()));
                }
            } else {
                // Print some sort of warning message - the header name
                // has no matched pair.
                // If we could, reset the previous token to undo the
                // change, but at present that's not possible and probably
                // not worth it
                advanceTokenizer = false;
            }
        break;
        case HttpTokenizer::HeaderValue:
            // Print some sort of warning message - HeaderValues should only
            // be found after a HeaderField
        break;
        case HttpTokenizer::HeaderComplete:
            // We don't really need to do anything here, this is just a heads up
        break;
        case HttpTokenizer::Content:
            // I'm still not sure if we should ever get content on a request.
            // Perhaps on a POST or something, but not on a GET I think.
        break;
        case HttpTokenizer::EndOfFile:
            // This should never be seen here, as hasToken() doesn't count this
            // as a token.
        break;
        case HttpTokenizer::NoConnection:
            // No connection could be read from the socket
            perror("Error reading socket");
            return;
        break;
        }
        if (advanceTokenizer) {
            tokenizer.next();
        }
        advanceTokenizer = true;
    }
}

HttpRequest::~HttpRequest() {
    // Nothing to destruct
}

void HttpRequest::setRequest(const std::string& req) {
    // The request has three parts: type uri version.
    // This picks out the first part.
    // Ex:
    // GET /index.php HTTP/1.0
    // 01234567890123456789012
    uint first = req.find(' ');
    if (first != std::string::npos) {
        // Parse out the request
        std::string request = req.substr(0, first);
        if (request.compare("GET") == 0) {
            type = HTTP_GET;
        } else {
            type = HTTP_UNKNOWN_REQUEST;
        }
        uint second = req.find(' ', first+1);
        if (second != std::string::npos) {
            // Parse out the uri
            std::string path = req.substr(first+1, second-first-1);
            setUri(path);
            // Parse out the version string
            std::string ver = req.substr(second+1);
            if (ver.compare("HTTP/1.1") == 0) {
                version = HTTP_1_1;
            } else if (ver.compare("HTTP/1.0") == 0) {
                version = HTTP_1_0;
            } else {
                version = HTTP_UNKNOWN_VERSION;
            }
        }
    }
}

void HttpRequest::setType(HttpRequestType requestType) {
    type = requestType;
}

HttpRequestType HttpRequest::getType() const {
    return type;
}

void HttpRequest::setUri(const std::string& path) {
    uri = path;
}

std::string HttpRequest::getUri() const {
    return uri;
}

void HttpRequest::setVersion(HttpVersion ver) {
    version = ver;
}

HttpVersion HttpRequest::getVersion() const {
    return version;
}

bool HttpRequest::hasHeader(const std::string& field) const {
    return header.find(field) == header.end();
}

void HttpRequest::addHeader(const std::string& field, const std::string& value) {
    header.insert(std::pair<std::string, std::string>(field, value));
}

std::string HttpRequest::getHeader(const std::string& field) const {
    if (hasHeader(field)) {
        return header.find(field)->second;
    } else {
        return "";
    }
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return header;
}

std::string HttpRequest::toString() const {
    std::string ret = requestTypeToString(type);
    ret.push_back(' ');
    ret.append(getUri());
    ret.push_back(' ');
    ret.append(versionToString(version));
    ret.append("\r\n");
    for (std::map<std::string, std::string>::const_iterator itr = header.begin(); itr != header.end(); itr++) {
        ret.append(itr->first);
        ret.append(": ");
        ret.append(itr->second);
        ret.append("\r\n");
    }
    ret.append("\r\n");
    return ret;
}

}