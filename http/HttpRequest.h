#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "http/HttpGlobal.h"

#include <string>
#include <map>

#include "http/HttpTokenizer.h"

namespace http {

/*
Example request:

GET /index.php HTTP/1.1\r\n
Host: www.example.com\r\n
User-Agent: MicrosoftIE/6.0 (Mozilla/2.0; Compatible)\r\n
\r\n


*/

class HttpRequest {
public:
    HttpRequest();
    HttpRequest(HttpTokenizer& tokenizer);
    ~HttpRequest();
    
    // This should automatically parse out the type, path, and version
    void setRequest(const std::string& req);
    
    // Request specific info
    void setType(HttpRequestType requestType);
    HttpRequestType getType() const;
    
    void setUri(const std::string& path);
    std::string getUri() const;
    
    void setVersion(HttpVersion ver);
    HttpVersion getVersion() const;
    
    bool hasHeader(const std::string& field) const;
    void addHeader(const std::string& field, const std::string& value);
    std::string getHeader(const std::string& field) const;
    const std::map<std::string, std::string>& getHeaders() const;
    
    std::string toString() const;
    
private:
    // GET is all that's supported for now
    HttpRequestType type;
    // Should probably start with a / character. If it doesn't, life
    // could get tricky. Note that ../ and ./ should be stripped for
    // security...but that sounds like more work now than it's worth.
    std::string uri;
    HttpVersion version;
    // Headers in a map
    std::map<std::string, std::string> header;
};

}

#endif
