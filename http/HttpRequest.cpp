#include "http/HttpRequest.h"
#include "http/HttpGlobal.h"
#include "http/HttpBufferedString.h"

#include <sstream>
#include <string>
#include <stdexcept>
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
    type(HTTP_UNKNOWN_REQUEST),
    uri("/"),
    version(HTTP_UNKNOWN_VERSION),
    prefixIndex(0),
    contentStarted(false)
{
    // Nothing else to construct
}

HttpRequest::HttpRequest(HttpTokenizer& tokenizer) :
    type(HTTP_UNKNOWN_REQUEST),
    uri("/"),
    version(HTTP_UNKNOWN_VERSION),
    prefixIndex(0),
    contentStarted(false)
{
    // Initialize via tokenizer
    //bool advanceTokenizer = true;
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
                //advanceTokenizer = false;
                continue;
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
        //if (advanceTokenizer) {
            tokenizer.next();
        //}
        //advanceTokenizer = true;
    }
}

HttpRequest::~HttpRequest() {
    // Nothing to destruct
}

void HttpRequest::setRequest(const std::string& req) {
    try {
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
            } else if (request.compare("POST") == 0) {
                type = HTTP_POST;
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
    } catch (std::out_of_range& e) {
        std::cout << "Error while parsing HTTP Request line" << std::endl;
    }
}

void HttpRequest::setType(HttpRequestType requestType) {
    type = requestType;
}

HttpRequestType HttpRequest::getType() const {
    return type;
}

void HttpRequest::setUri(const std::string& path) {
    size_t queryStart = path.find('?');
    size_t extStart = path.find_last_of('.', queryStart);
    if (extStart != std::string::npos) {
        if (queryStart != std::string::npos) {
            ext = path.substr(extStart+1, queryStart - extStart - 1);
        } else {
            ext = path.substr(extStart+1);
        }
    } else {
        ext = "";
    }
    if (queryStart != std::string::npos) {
        // Break the query section off of the uri
        uri = path.substr(0, queryStart);
        setParameters(path.substr(queryStart+1));
    } else {
        uri = path;
    }
}

std::string HttpRequest::getUri() const {
    return uri;
}

std::string HttpRequest::getExt() const {
    return ext;
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

void HttpRequest::removeHeader(const std::string& field) {
    header.erase(field);
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

void HttpRequest::setParameters(const std::string& params) {
    unsigned int start = 0;
    unsigned int end = 0;
    while (start < params.size()) {
        // Ex: p1=v1&p2=v2&p3=v3
        // Search for a parameter name
        std::string param;
        while (end < params.size() && params.at(end) != '=') {
            end++;
        }
        param = params.substr(start, end-start);
        
        // Search for a parameter value
        std::string value;
        if (end < params.size()) {
            end++;
            start = end;
            while (end < params.size() && params.at(end) != '&') {
                end++;
            }
            value = params.substr(start, end-start);
            parameters.insert(std::pair<std::string, std::string>(param, value));
        }
        end++;
        start = end;
    }
}

std::string HttpRequest::getParameterString() const {
    std::string ret;
    for (std::map<std::string, std::string>::const_iterator itr = parameters.begin(); itr != parameters.end(); itr++) {
        if (itr != parameters.begin()) {
            ret.push_back('&');
        }
        ret.append(itr->first);
        ret.push_back('=');
        ret.append(itr->second);
    }
    return ret;
}

bool HttpRequest::hasParameter(const std::string& param) const {
    return parameters.find(param) == parameters.end();
}

void HttpRequest::addParameter(const std::string& param, const std::string& value) {
    parameters.insert(std::pair<std::string, std::string>(param, value));
}

std::string HttpRequest::getParameter(const std::string& param) const {
    if (hasParameter(param)) {
        return parameters.find(param)->second;
    } else {
        return "";
    }
}

const std::map<std::string, std::string>& HttpRequest::getParameters() const {
    return parameters;
}

//void HttpRequest::setContent(boost::shared_ptr<HttpBufferedContent> cont) {
//    content = cont;
//}

void HttpRequest::resetStream() {
    prefixIndex = 0;
    contentStarted = false;
    prefix = requestTypeToString(type);
    std::string params(getParameterString()); // Cache the param string
    // Reserve the first bit
    prefix.reserve(prefix.size() + 1 + getUri().size() + (type == HTTP_GET ? 1 + params.size() : 0) + 1 + versionToString(version).size() + 2);
    prefix.push_back(' ');
    prefix.append(getUri());
    if (type == HTTP_GET) {
        prefix.push_back('?');
        prefix.append(params);
    }
    prefix.push_back(' ');
    prefix.append(versionToString(version));
    prefix.append("\r\n");
    for (std::map<std::string, std::string>::const_iterator itr = header.begin(); itr != header.end(); itr++) {
        if (itr->first.compare("Content-Length") != 0) {
            // Reserve this loop's length
            prefix.reserve(prefix.size() + itr->first.size() + 2 + itr->second.size() + 2);
            prefix.append(itr->first);
            prefix.append(": ");
            prefix.append(itr->second);
            prefix.append("\r\n");
        }
    }
    boost::shared_ptr<HttpBufferedContent> cont(new HttpBufferedString(getParameterString()));
    content.swap(cont);
}

bool HttpRequest::streamComplete() const {
    if (prefixIndex >= prefix.size()) {
        if (content) {
            return content->streamComplete();
        } else {
            return contentStarted;
        }
    } else {
        return false;
    }
}

std::string HttpRequest::readStream(int size) {
    if (prefixIndex >= prefix.size()) {
        contentStarted = true;
        if (content) {
            return content->readStream(size);
        } else {
            return "\r\n";
        }
    } else {
        // Make sure reset has been called at least once to initialize the string
        resetStream();
        std::string ret;
        ret = prefix.substr(prefixIndex, size);
        prefixIndex += ret.size();
        return ret;
    }
}

}