#include "http/HttpServerThread.h"
#include "http/HttpWorkerThread.h"
#include "http/HttpGlobal.h"

#include <iostream>
#include <cstdio>

namespace http {

HttpServerThread::HttpServerThread(HttpServer* serv) :
    server(serv)
{
    // Nothing to create here.
    // server is the only state-wise variable - all others are local
    // so operator()() is threadsafe.
}

HttpServerThread::~HttpServerThread() {
    // Nothing to destroy
}

void HttpServerThread::operator()() {
    // Only reading, so no need to lock the active state.
    // Also, that would prevent anything else from locking it.
    while (server->active()) {
        std::cout << "HttpServerThread::operator()()" << std::endl;
        socklen_t socklen = (socklen_t)sizeof(sockaddr);
        int socket = accept(server->getServerSocket(), server->getAddress(), &socklen);
        std::cout << "HttpServerThread::operator()() accepted!" << std::endl;
        if (socket != SOCKET_ERROR) {
            boost::unique_lock<boost::shared_mutex> lock(server->getQueueMutex());
            if (server->getThreadpool().size() < server->maxThreadCount) {
                boost::shared_ptr<boost::thread> worker(new boost::thread(HttpWorkerThread(server)));
                server->getThreadpool().insert(worker);
            }
            // Set the timeout for the new socket
            setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, server->getTimeout(), sizeof(timeval));
            setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, server->getTimeout(), sizeof(timeval));
            server->getQueue().push(socket);
        }
    }
    std::cout << "/HttpServerThread::operator()()" << std::endl;
    for (std::set<boost::shared_ptr<boost::thread> >::iterator itr = server->getThreadpool().begin(); itr != server->getThreadpool().end(); itr++) {
        (*itr)->join();
    }
    server->getThreadpool().clear();
    if (close(server->getServerSocket()) == SOCKET_ERROR) {
        perror("Error closing server socket");
        //return;
    }
}

}