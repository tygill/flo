#include "http/HttpServer.h"

#include <iostream>
#include <cstdio>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

#include "http/HttpGlobal.h"
#include "http/HttpServerThread.h"
#include "http/HttpWorkerThread.h"

namespace http {

HttpServer::HttpServer(std::string htdocPath, ushort hostPort) :
    isActive(false),
    serverThread(NULL),
    server(this),
    port(hostPort),
    root()
{
    setRoot(htdocPath);
    //std::cout << "HttpServer::HttpServer()" << std::endl;
    // Nothing else to initialize
}

HttpServer::HttpServer(const HttpServer& other) :
    isActive(other.isActive),
    stateMutex(),
    serverThread(NULL),
    server(other.server),
    port(other.port),
    root(other.root)
{
    // Implemented here to prevent compiler errors due to uncopyable nature of mutexes
}

HttpServer::~HttpServer() {
    //std::cout << "HttpServer::~HttpServer()" << std::endl;
    // Clean up everything (threads, port listening, etc.)
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    if (active()) {
        stop();
    }
    //std::cout << "/HttpServer::~HttpServer()" << std::endl;
}

bool HttpServer::start() {
    //std::cout << "HttpServer::start()" << std::endl;
    // See if we're active first
    //if (!active()) {
        // Lock the state so we don't lose the fact that we were activated.
        boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
        // Make sure we didn't become active in the time it took to create the lock.
        if (!active()) {
            {
                boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
                isActive = true;
            }
            
            boost::scoped_ptr<boost::thread> tmp(new boost::thread(boost::ref(server)));
            server.initialize();
            if (server.valid()) {
                serverThread.swap(tmp);
            } else {
                return false;
            }
        }
    //}
    // If we made it this far, we succeeded.
    //std::cout << "/HttpServer::start()" << std::endl;
    return true;
}

void HttpServer::stop() {
    //std::cout << "HttpServer::stop()" << std::endl;
    boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
    if (active()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
        isActive = false;
        serverThread->join();
    }
    //std::cout << "/HttpServer::stop()" << std::endl;
}

bool HttpServer::active() const {
    //std::cout << "HttpServer::active()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    //std::cout << "/HttpServer::active()" << std::endl;
    return isActive;
}

void HttpServer::setPort(ushort hostPort) {
    //std::cout << "HttpServer::setPort()" << std::endl;
    boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
    //std::cout << "/HttpServer::setPort()" << std::endl;
    if (!active()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
        port = hostPort;
    }
}

ushort HttpServer::getPort() const {
    //std::cout << "HttpServer::getPort()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    //std::cout << "/HttpServer::getPort()" << std::endl;
    return port;
}

void HttpServer::setRoot(const std::string& htdocPath) {
    //std::cout << "HttpServer::setRoot()" << std::endl;
    boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
    if (!active()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
        root = htdocPath;
        // Ensures that root never ends with a trailing /
        // Ex: /home/tyler/htdocs
        try {
            if (root.at(root.size()-1) == '/') {
                root.erase(root.size()-1);
            }
        } catch (std::out_of_range& e) {
            std::cerr << "The root directory of the server has been set to the root directory.\n";
            std::cerr << "Generally, you don't want that to happen." << std::endl;
        }
    }
    //std::cout << "/HttpServer::setRoot()" << std::endl;
}

std::string HttpServer::getRoot() const {
    //std::cout << "HttpServer::getRoot()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    //std::cout << "/HttpServer::getRoot()" << std::endl;
    return root;
}

}
