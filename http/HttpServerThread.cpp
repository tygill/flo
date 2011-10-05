#include "http/HttpServerThread.h"

#include "http/HttpServer.h"
#include "http/HttpWorkerThread.h"
#include "http/HttpGlobal.h"

#include <iostream>
#include <cstdio>

namespace http {

HttpServerThread::HttpServerThread(HttpServer* serv) :
    server(serv),
    serverSocket(-1),
    maxThreadCount(MAX_THREAD_COUNT * boost::thread::hardware_concurrency()),
    isValid(false)
{
    
}

HttpServerThread::HttpServerThread(const HttpServerThread& other) :
    server(other.server),
    serverSocket(other.serverSocket),
    timeout(other.timeout),
    address(other.address),
    queue(),
    queueMutex(),
    threadpool(),
    maxThreadCount(other.maxThreadCount),
    isValid(other.isValid)
{
    
}

HttpServerThread::~HttpServerThread() {
    // Nothing to destroy
}

void HttpServerThread::initialize() {
    isValid = true;
    root = server->getRoot();
    
    // Make the socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == SOCKET_ERROR) {
        perror("Error creating server socket");
        isValid = false;
    }

    // Setup a timeout object
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    // Set the timeout
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(serverSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Setup the address
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server->getPort());
    address.sin_family = AF_INET;
    
    // Bind a socket
    if (bind(serverSocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        perror("Error binding server socket");
        isValid = false;
    }
    
    // Load new address data
    socklen_t socklen = (socklen_t)sizeof(address);
    getsockname(serverSocket, (sockaddr*)&address, &socklen);
    
    // Start listening
    if (listen(serverSocket, 25) == SOCKET_ERROR) {
        perror("Error listening on server socket");
        isValid = false;
    }
}

void HttpServerThread::operator()() {
    // Only reading, so no need to lock the active state.
    // Also, that would prevent anything else from locking it.
    while (server->active()) {
        //std::cout << "HttpServerThread::operator()()" << std::endl;
        socklen_t socklen = (socklen_t)sizeof(sockaddr);
        int socket = accept(serverSocket, (sockaddr*)&address, &socklen);
        //std::cout << "HttpServerThread::operator()() accepted!" << std::endl;
        if (socket != SOCKET_ERROR) {
            // Set the timeout for the new socket
            setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval));
            setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeval));
            {
                boost::unique_lock<boost::shared_mutex> lock(queueMutex);
                queue.push(socket);
                //std::cout << "Items in queue: " << queue.size() << std::endl;
            }
            //std::cout << ":)" << std::endl;
            if (threadpool.size() < maxThreadCount) {
                boost::shared_ptr<boost::thread> worker(new boost::thread(HttpWorkerThread(this)));
                threadpool.insert(worker);
            }
        }
    }
    //std::cout << "/HttpServerThread::operator()()" << std::endl;
    for (std::set<boost::shared_ptr<boost::thread> >::iterator itr = threadpool.begin(); itr != threadpool.end(); itr++) {
        (*itr)->join();
    }
    threadpool.clear();
    if (close(serverSocket) == SOCKET_ERROR) {
        perror("Error closing server socket");
        //return;
    }
    isValid = false;
}

bool HttpServerThread::active() const {
    return isValid && server->active();
}

bool HttpServerThread::valid() const {
    return isValid;
}

std::string HttpServerThread::getRoot() const {
    return root;
}

int HttpServerThread::threadCount() const {
    return threadpool.size();
}

std::queue<int>& HttpServerThread::getQueue() {
    return queue;
}

boost::shared_mutex& HttpServerThread::getQueueMutex() {
    return queueMutex;
}

}