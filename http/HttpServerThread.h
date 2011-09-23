#ifndef HTTPSERVERTHREAD_H
#define HTTPSERVERTHREAD_H

#include "http/HttpServer.h"

namespace http {

class HttpServer;

class HttpServerThread {
public:
    HttpServerThread(HttpServer* serv);
    ~HttpServerThread();
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
    
    // Gets the number of threads currently in the threadpool
    int threadCount() const;
private:
    HttpServer* server;
    int serverSocket;
    timeval timeout;
    sockaddr_in address;z
    std::set<boost::shared_ptr<boost::thread> > threadpool;
    uint maxThreadCount;
};

}

#endif
