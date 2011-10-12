﻿#include "http/HttpResponse.h"
#include "http/HttpBufferedContent.h"
#include "http/HttpBufferedString.h"

#include <sstream>
#include <iostream>
#include <cstdio>
#include <cctype>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>

namespace http {

/*
boost::shared_ptr<HttpResponse> load(const std::string& host, ushort destPort, const std::string& uri) {
int error;
    boost::shared_ptr<HttpResponse> ret;
    
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == SOCKET_ERROR) {
        perror("Error creating socket");
        // Print more information about the error
        return ret;
    }
    
    // This can't really be wrapped in a smart_ptr, as it souldn't free
    // the linked list chain of subsequent addrinfo's on a delete.
    // So hopefully there are no errors between here and the freeaddrinfo()
    addrinfo* info = NULL;
    error = getaddrinfo(host.c_str(), NULL, NULL, &info);
    if (error) {
        // Print host resolution error
        std::cout << "Error resolving address: " << gai_strerror(error) << std::endl;
        return ret;
    }
    
    // I'm not going to do ANY IP6 checking or code here, so sorry.
    // Cast to sockaddr_in instead of leaving it as a plain sockaddr
    //  That way we can extract the IP4 address
    sockaddr_in* address = (sockaddr_in*)info->ai_addr;
    
    // address might have already loaded port and family info, but we'll
    // overwrite it with whatever we got from the commandline
    // address->sin_addr.s_addr is already loaded from getaddrinfo()
    address->sin_port = htons(destPort);
    address->sin_family = AF_INET;
    
    // ntohl(address->sin_addr.s_addr)
    //std::cout << "Connecting to " << host << ":" << ntohs(address->sin_port) << uri << std::endl;
    
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    if (connect(sock, (sockaddr*)address, sizeof(sockaddr_in)) == SOCKET_ERROR) {
        // Print connection error
        std::cout << "Error connecting to socket: Connection timed out" << std::endl;
        freeaddrinfo(info);
        return ret;
    }
    
    // Build the HTTP/1.0 request:
    // "GET /path/to/file/index.html HTTP/1.0\r\nHost: hostname\r\n\r\n"
    std::string request("GET ");
    request.append(uri);
    request.append(" HTTP/1.1\r\nHost: ");
    request.append(host);
    request.append(":");
    std::ostringstream osstream;
    osstream << destPort;
    request.append(osstream.str());
    request.append("\r\n\r\n");
    //std::cout << request << std::endl;
    // Does this need +1?
    if (write(sock, request.c_str(), request.length()) == SOCKET_ERROR) {
        perror("Error writing to socket");
        freeaddrinfo(info);
        return ret;
    }
    
    ret = boost::shared_ptr<HttpResponse>(new HttpResponse(request));
    HttpTokenizer tokenizer(sock);
    
    bool advanceTokenizer = true;
    while (tokenizer.hasToken()) {
        std::string name;
        switch (tokenizer.tokenType()) {
        case HttpTokenizer::HttpStatus:
            // If we needed to redirect or anything, here's where we could
            // do that, and return a newly loaded page, which could then
            // even function recursively.
            // If so desired, this could also parse out the status message
            // and put it into the HttpResponse, which would actually be
            // a really good idea.
            ret->setStatus(tokenizer.tokenValue());
            //std::cout << "Status:" << std::endl;
            //std::cout << tokenizer.tokenValue() << std::endl;
        break;
        case HttpTokenizer::HttpRequest:
            // We shouldn't be seeing this, as a response
            // should start with a status, not a request...
        break;
        case HttpTokenizer::HeaderField:
            // We're going to grab the next token, and make sure it's a
            // HeaderValue token, so store the HeaderField
            name = tokenizer.tokenValue();
            tokenizer.next();
            if (tokenizer.hasToken() && tokenizer.tokenType() == HttpTokenizer::HeaderValue) {
                ret->addHeader(name, tokenizer.tokenValue());
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
            // We should never get multiple contents back from a single tokenizer
            ret->setContent(tokenizer.tokenValue());
            //std::cout << "Content:" << std::endl;
            //std::cout << "Length: " << tokenizer.tokenValue().length() << std::endl;
            //std::cout << tokenizer.tokenValue() << std::endl;
        break;
        case HttpTokenizer::EndOfFile:
            // This should never be seen here, as hasToken() doesn't count this
            // as a token.
        break;
        case HttpTokenizer::NoConnection:
            // No connection could be read from the socket
            perror("Error reading socket");
            freeaddrinfo(info);
            return ret;
        break;
        }
        if (advanceTokenizer) {
            tokenizer.next();
        }
        advanceTokenizer = true;
    }
    
    freeaddrinfo(info);
    return ret;
}
*/

HttpResponse::HttpResponse() :
    version(HTTP_UNKNOWN_VERSION),
    status(400),
    prefixIndex(0),
    contentStarted(false)
{
    // Don't need to build anything else
}

HttpResponse::HttpResponse(HttpTokenizer& tokenizer) :
    version(HTTP_UNKNOWN_VERSION),
    status(400),
    prefixIndex(0),
    contentStarted(false)
{
    // Initialize via tokenizer
    bool advanceTokenizer = true;
    while (tokenizer.hasToken()) {
        std::string name;
        switch (tokenizer.tokenType()) {
        case HttpTokenizer::HttpStatus:
            // If we needed to redirect or anything, here's where we could
            // do that, and return a newly loaded page, which could then
            // even function recursively.
            // If so desired, this could also parse out the status message
            // and put it into the HttpResponse, which would actually be
            // a really good idea.
            setStatus(tokenizer.tokenValue());
            //std::cout << "Status:" << std::endl;
            //std::cout << tokenizer.tokenValue() << std::endl;
        break;
        case HttpTokenizer::HttpRequest:
            // We shouldn't be seeing this, as a response
            // should start with a status, not a request...
        break;
        case HttpTokenizer::HeaderField:
            // We're going to grab the next token, and make sure it's a
            // HeaderValue token, so store the HeaderField
            name = tokenizer.tokenValue();
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
            // We should never get multiple contents back from a single tokenizer
            {
                boost::shared_ptr<HttpBufferedContent> content(new HttpBufferedString(tokenizer.tokenValue()));
                setContent(content);
            }
            //std::cout << "Content:" << std::endl;
            //std::cout << "Length: " << tokenizer.tokenValue().length() << std::endl;
            //std::cout << tokenizer.tokenValue() << std::endl;
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

HttpResponse::~HttpResponse() {
    // Nothing to destroy
}

void HttpResponse::setStatus(const std::string& stat) {
    // Extract the status information from the status line
    // Ex:
    // HTTP/1.1 200 OK
    // 012345678901234
    size_t first = stat.find(' ');
    if (first != std::string::npos) {
        std::string ver = stat.substr(0, first);
        if (ver.compare("HTTP/1.1") == 0) {
            version = HTTP_1_1;
        } else if (ver.compare("HTTP/1.0") == 0) {
            version = HTTP_1_0;
        } else {
            version = HTTP_UNKNOWN_VERSION;
        }
        size_t second = stat.find(' ', first+1);
        if (second != std::string::npos) {
            std::string statusCode = stat.substr(first+1, second-first-1);
            status = atoi(statusCode.c_str());
        }
    }
}

void HttpResponse::setVersion(HttpVersion ver) {
    version = ver;
}

HttpVersion HttpResponse::getVersion() const {
    return version;
}

void HttpResponse::setStatusCode(int code) {
    status = code;
}

int HttpResponse::getStatusCode() const {
    return status;
}

bool HttpResponse::hasHeader(const std::string& field) const {
    return header.find(field) == header.end();
}

void HttpResponse::addHeader(const std::string& field, const std::string& value) {
    header.insert(std::pair<std::string, std::string>(field, value));
}

std::string HttpResponse::getHeader(const std::string& field) const {
    if (hasHeader(field)) {
        return header.find(field)->second;
    } else {
        return "";
    }
}

const std::map<std::string, std::string>& HttpResponse::getHeaders() const {
    return header;
}

void HttpResponse::setContent(boost::shared_ptr<HttpBufferedContent> cont) {
    content = cont;
}

void HttpResponse::resetStream() {
    prefixIndex = 0;
    contentStarted = false;
    std::stringstream code;
    code << status;
    
    // Build the prefix to the content
    // Should include server headers, but should not end the header
    prefix.reserve(versionToString(version).size() + 1 + code.str().size() + 1 + statusCodeToString(status).size() + 2);
    prefix = versionToString(version);
    prefix.push_back(' ');
    prefix.append(code.str());
    prefix.push_back(' ');
    prefix.append(statusCodeToString(status));
    prefix.append("\r\n");
    for (std::map<std::string, std::string>::const_iterator itr = header.begin(); itr != header.end(); itr++) {
        if (itr->first.compare("Content-Length") != 0) {
            // Reserve this loops return string
            prefix.reserve(prefix.size() + itr->first.size() + 2 + itr->second.size() + 2);
            prefix.append(itr->first);
            prefix.append(": ");
            prefix.append(itr->second);
            prefix.append("\r\n");
        }
    }
    
    if (content) {
        content->resetStream();
    }
}

bool HttpResponse::streamComplete() const {
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

std::string HttpResponse::readStream(int size) {
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
