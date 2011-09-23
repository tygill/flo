#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <queue>
#include <set>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

namespace http {

class HttpServerThread;
class HttpWorkerThread;

// The HttpServer will be designed to run in a separate thread from main, and will
// spawn worker threads as needed. This way, the main thread can still have control
// enough to kill the server when needed. The server itself will be created as the
// new thread (via operator()()), to simplify passing of data around.
// The server thread will basically just be a constant accept() loop.
// start() and stop() should thusly be threadsafe.
// get and set methods will silently fail if the server is running
// it is restarted.
class HttpServer {
public:
    HttpServer(std::string htdocPath, ushort hostPort = 80, uint maxThread = 25);
private:
    HttpServer(const HttpServer& other);
public:
    ~HttpServer();
    
    // Starts the server
    bool start(); // stateMutex locked
    void stop(); // stateMutex locked
    
    bool active() const; // stateMutex shared
    
    // Configuration - must be called before start() is called.
    // If the server is already running, then the modifiers do nothing.
    void setPort(ushort hostPort);
    ushort getPort() const;
    void setRoot(const std::string& htdocPath);
    std::string getRoot() const;
    
    // Gets the number of threads currently in the threadpool
    int threadCount() const;
    // Adds a thread to the threadpool
    //void addThread();
private:
    // The accept loop
    //void operator()();
    
    friend class http::HttpServerThread;
    friend class http::HttpWorkerThread;
    
    // Server thread access functions
    int getServerSocket();
    sockaddr* getAddress();
    timeval* getTimeout();
    std::set<boost::shared_ptr<boost::thread> >& getThreadpool();
    // Server worker thread access functions
    std::queue<int>& getQueue();
    boost::shared_mutex & getQueueMutex();
    
    bool isActive;
    mutable boost::shared_mutex stateMutex;
    
    int serverSocket;
    boost::scoped_ptr<boost::thread> serverThread;
    timeval timeout;
    sockaddr_in address;
    ushort port;
    std::string root;
    
    std::queue<int> queue;
    boost::shared_mutex  queueMutex;
    std::set<boost::shared_ptr<boost::thread> > threadpool;
    uint maxThreadCount;
};

}

#endif
