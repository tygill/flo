#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

#include "http/HttpGlobal.h"
#include "http/HttpTokenizer.h"

namespace http {

class HttpResponse {
public:
    HttpResponse();
    HttpResponse(HttpTokenizer& tokenizer);
    ~HttpResponse();
    
    /*
    // Request access
    void setRequest(boost::shared_ptr<HttpRequest> req);
    boost::shared_ptr<HttpRequest> getRequest() const;
    */
    
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
    void setContent(const std::string& cont);
    std::string getContent() const;
    
    // Get the whole thing to send to the socket
    std::string toString() const;
    
private:
    //boost::shared_ptr<HttpRequest> request;
    HttpVersion version;
    int status;
    std::map<std::string, std::string> header;
    std::string content;
};

}

#endif
