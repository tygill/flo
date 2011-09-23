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

HttpServer::HttpServer(std::string htdocPath, ushort hostPort, uint maxThread) :
    isActive(false),
    serverSocket(-1),
    serverThread(NULL),
    port(hostPort),
    root(htdocPath),
    maxThreadCount(maxThread)
{
    std::cout << "HttpServer::HttpServer()" << std::endl;
    // Nothing else to initialize
}

HttpServer::HttpServer(const HttpServer& other) :
    isActive(other.isActive),
    stateMutex(),
    serverSocket(-1),
    serverThread(NULL),
    timeout(other.timeout),
    address(other.address),
    port(other.port),
    root(other.root),
    queue(),
    queueMutex(),
    threadpool(),
    maxThreadCount(other.maxThreadCount)
{
    
}

HttpServer::~HttpServer() {
    std::cout << "HttpServer::~HttpServer()" << std::endl;
    // Clean up everything (threads, port listening, etc.)
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    if (active()) {
        stop();
    }
    std::cout << "/HttpServer::~HttpServer()" << std::endl;
}

bool HttpServer::start() {
    std::cout << "HttpServer::start()" << std::endl;
    // See if we're active first
    //if (!active()) {
        // Lock the state so we don't lose the fact that we were activated.
        boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
        // Make sure we didn't become active in the time it took to create the lock.
        if (!active()) {
            boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
            isActive = true;
            
            // Make the socket
            serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (serverSocket == SOCKET_ERROR) {
                perror("Error creating server socket");
                return false;
            }

            // Setup a timeout object
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
            
            // Set the timeout
            setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(serverSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            
            // Setup the address
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port);
            address.sin_family = AF_INET;
            
            // Bind a socket
            if (bind(serverSocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
                perror("Error binding server socket");
                return false;
            }
            
            // Load new address data
            socklen_t socklen = (socklen_t)sizeof(address);
            getsockname(serverSocket, (sockaddr*)&address, &socklen);
            
            // Start listening
            if (listen(serverSocket, 25) == SOCKET_ERROR) {
                perror("Error listening on server socket");
                return false;
            }
            
            boost::scoped_ptr<boost::thread> tmp(new boost::thread(HttpServerThread(this)));
            serverThread.swap(tmp);
        }
    //}
    // If we made it this far, we succeeded.
    std::cout << "/HttpServer::start()" << std::endl;
    return true;
}

void HttpServer::stop() {
    std::cout << "HttpServer::stop()" << std::endl;
    boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
    if (active()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
        isActive = false;
        serverThread->join();
        serverSocket = -1;
    }
    std::cout << "/HttpServer::stop()" << std::endl;
}

bool HttpServer::active() const {
    std::cout << "HttpServer::active()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    std::cout << "/HttpServer::active()" << std::endl;
    return isActive;
}

void HttpServer::setPort(ushort hostPort) {
    std::cout << "HttpServer::setPort()" << std::endl;
    boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
    std::cout << "/HttpServer::setPort()" << std::endl;
    if (!active()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
        port = hostPort;
    }
}

ushort HttpServer::getPort() const {
    std::cout << "HttpServer::getPort()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    std::cout << "/HttpServer::getPort()" << std::endl;
    return port;
}

void HttpServer::setRoot(const std::string& htdocPath) {
    std::cout << "HttpServer::setRoot()" << std::endl;
    boost::upgrade_lock<boost::shared_mutex> lock(stateMutex);
    if (!active()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
        root = htdocPath;
    }
    std::cout << "/HttpServer::setRoot()" << std::endl;
}

std::string HttpServer::getRoot() const {
    std::cout << "HttpServer::getRoot()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    std::cout << "/HttpServer::getRoot()" << std::endl;
    return root;
}

int HttpServer::threadCount() const {
    std::cout << "HttpServer::threadCount()" << std::endl;
    boost::shared_lock<boost::shared_mutex> lock(stateMutex);
    std::cout << "/HttpServer::threadCount()" << std::endl;
    return queue.size();
}

/*
void HttpServer::addThread() {
    boost::upgrade_lock lock(stateMutex);
    if (active() && threadpool.size() < maxThreadCount) {
        boost::upgrade_to_unique_lock unique(lock);
        boost::thread worker(*workerThread);
        threadpool.insert(worker);
    }
}
*/

int HttpServer::getServerSocket() {
    return serverSocket;
}

sockaddr* HttpServer::getAddress() {
    return (sockaddr*)&address;
}

timeval* HttpServer::getTimeout() {
    return &timeout;
}

std::set<boost::shared_ptr<boost::thread> >& HttpServer::getThreadpool() {
    return threadpool;
}

std::queue<int>& HttpServer::getQueue() {
    return queue;
}

boost::shared_mutex& HttpServer::getQueueMutex() {
    return queueMutex;
}

}
