#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

#include "http/HttpGlobal.h"
#include "http/HttpTokenizer.h"
#include "http/HttpBufferedContent.h"

namespace http {

class HttpResponse {
public:
    HttpResponse();
    HttpResponse(HttpTokenizer& tokenizer);
    ~HttpResponse();
    
    // This should now automatically parse out the version, status code,
    // and status text
    void setStatus(const std::string& stat);
    
    // Status modifiers
    void setVersion(HttpVersion ver);
    HttpVersion getVersion() const;
    
    void setStatusCode(int code);
    int getStatusCode() const;
    
    // Header manipulation functions
    bool hasHeader(const std::string& field) const;
    void addHeader(const std::string& field, const std::string& value);
    std::string getHeader(const std::string& field) const;
    const std::map<std::string, std::string>& getHeaders() const;
    
    // Content manipulations
    void setContent(boost::shared_ptr<HttpBufferedContent> cont);
    
    // Allow more bufferable toString()
    // This should allow readStream() to encapsulate file loading in a
    // slowly buffered way. That buffering responsibility will be
    // passed along to a HttpBufferedContent object though.
    // Then that HttpBufferedContent virtual class will provide file,
    // directory, and cgi implementations.
    void resetStream();
    bool streamComplete() const;
    std::string readStream(int size = 1024);
    // With this streaming implementation, headers will be interesting.
    // The best way to do it would probably be run the buffered contents
    // with the assumption that the HTTP status line and possibly some
    // headers have been sent, but that they are responsible for sending
    // the header terminator before sending the content. Content-Length,
    // Content-Type, etc. should be put by the buffered file as well.
    
    // With this streaming approach, this is how the socket could be
    // written:
    /*
    std::string str;
    response->resetStream();
    while (!response->streamComplete()) {
        str = response->readStream(1024);
        write(sock, str.c_str(), str.size());
    }
    shutdown(sock, SHUT_RDWR);
    if (close(sock) == SOCKET_ERROR) {
        perror("Error closing socket");
    }
    */
    
private:
    HttpVersion version;
    int status;
    std::map<std::string, std::string> header;
    std::string prefix;
    unsigned int prefixIndex;
    bool contentStarted;
    boost::shared_ptr<HttpBufferedContent> content;
};

}

#endif
