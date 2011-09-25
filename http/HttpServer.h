#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <boost/thread.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

namespace http {

class HttpServer;

}

#include "http/HttpServerThread.h"

namespace http {

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
    HttpServer(std::string htdocPath, ushort hostPort = 80);
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
    
private:
    
    bool isActive;
    mutable boost::shared_mutex stateMutex;
    
    boost::scoped_ptr<boost::thread> serverThread;
    HttpServerThread server;
    ushort port;
    std::string root;
};

}

#endif
