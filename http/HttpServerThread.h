#ifndef HTTPSERVERTHREAD_H
#define HTTPSERVERTHREAD_H

#include <queue>
#include <set>
#include <boost/thread.hpp>
#include <boost/weak_ptr.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

namespace http {

class HttpServer;

class HttpServerThread {
public:
    HttpServerThread(HttpServer* serv);
    HttpServerThread(const HttpServerThread& other);
    ~HttpServerThread();
    void initialize();
    
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
    
    bool active() const;
    
    bool valid() const;
    
    std::string getRoot() const;
    // Gets the number of threads currently in the threadpool
    int threadCount() const;
    
    std::queue<int>& getQueue();
    boost::shared_mutex& getQueueMutex();
private:
    HttpServer* server;
    int serverSocket;
    timeval timeout;
    sockaddr_in address;
    
    std::string root;
    
    std::queue<int> queue;
    boost::shared_mutex queueMutex;
    std::set<boost::shared_ptr<boost::thread> > threadpool;
    uint maxThreadCount;
    
    bool isValid;
};

}

#endif
