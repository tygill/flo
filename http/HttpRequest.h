#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "http/HttpGlobal.h"
#include "http/HttpTokenizer.h"
#include "http/HttpBufferedContent.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <map>

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
    
    // This also handles parsing out any get parameters
    void setUri(const std::string& path);
    std::string getUri() const;
    std::string getExt() const;
    
    void setVersion(HttpVersion ver);
    HttpVersion getVersion() const;
    
    bool hasHeader(const std::string& field) const;
    void addHeader(const std::string& field, const std::string& value);
    void removeHeader(const std::string& field);
    std::string getHeader(const std::string& field) const;
    const std::map<std::string, std::string>& getHeaders() const;
    
    // This parses out a list of parameters and adds them all, from the format p1=v1&p2=v2
    void setParameters(const std::string& params);
    std::string getParameterString() const;
    
    bool hasParameter(const std::string& param) const;
    void addParameter(const std::string& param, const std::string& value);
    std::string getParameter(const std::string& param) const;
    const std::map<std::string, std::string>& getParameters() const;
    
    // Content manipulations
    //void setContent(boost::shared_ptr<HttpBufferedContent> cont);
    
    void resetStream();
    bool streamComplete() const;
    std::string readStream(int size = 1024);
    
private:
    // GET is all that's supported for now
    HttpRequestType type;
    // Should probably start with a / character. If it doesn't, life
    // could get tricky. Note that ../ and ./ should be stripped for
    // security...but that sounds like more work now than it's worth.
    std::string uri;
    std::string ext;
    HttpVersion version;
    // Headers in a map
    std::map<std::string, std::string> header;
    std::map<std::string, std::string> parameters;
    std::string prefix;
    unsigned int prefixIndex;
    bool contentStarted;
    boost::shared_ptr<HttpBufferedContent> content;
};

}

#endif
