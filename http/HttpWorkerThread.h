#ifndef HTTPWORKERTHREAD_H
#define HTTPWORKERTHREAD_H

#include <boost/shared_ptr.hpp>

#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpGlobal.h"

namespace http {

class HttpServerThread;

class HttpWorkerThread {
public:
    HttpWorkerThread(HttpServerThread* server);
    ~HttpWorkerThread();
    void operator()();
    // Checks server->active() to decide whether or not to stay alive.
    // Things are locked in this manner:
    //  Read access only needed:
    //   boost::shared_lock<boost::shared_mutex> lock(server->getMutex());
    //  May need to write: (unless all are this way - then it's pointless)
    //   boost::upgrade_lock<boost::shared_mutex> lock(server->getMutex());
    //    boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
    //  Write access:
    //   boost::unique_lock<boost::shared_mutex> lock(server->getMutex());
    
    // HttpResponse will probably need some upgrades as well, as once the
    // request has been parsed, we can just create a new HttpResponse and
    // then populate it into the size it should be, then start sending it
    // along the wire. Hopefully there aren't problems with std::string &
    // loading jpg or gif content in...but this would make giving out the
    // Content-Length really easy.
private:
    // Services a request
    // First, it determines the type of request (GET, POST, etc.), and
    // fails silently if the request type is unsupported.
    boost::shared_ptr<HttpResponse> service(const HttpRequest* request) const;
    
    // Handles GET requests
    // There are two types of requests that might be made - getting a
    // file or getting a directory. Getting a directory requires reading
    // a directory and building an html page listing for it.
    // The content type will be looked up from another function here
    // as well.
    boost::shared_ptr<HttpResponse> get(const HttpRequest* request) const;
    
    // GETs a specific file
    boost::shared_ptr<HttpResponse> getFile(const std::string& uri) const;
    
    // GETs a directory listing
    boost::shared_ptr<HttpResponse> getDirectory(const std::string& uri) const;
    
    // Returns an error response (possibly loading an error file?)
    boost::shared_ptr<HttpResponse> error(int statusCode) const;
    
    static std::string getContentType(const std::string& extension);
    
    HttpServerThread* serverThread;
};

}

#endif
