#include "http/HttpWorkerThread.h"
#include "http/HttpServer.h"

#include <iostream>
#include <unistd.h>

namespace http {

HttpWorkerThread::HttpWorkerThread(HttpServer* serv) :
    server(serv)
{
    // Nothing to create here.
    // server is the only state-wise variable - all others are local
    // so operator()() is threadsafe.
}

HttpWorkerThread::~HttpWorkerThread() {
    // Nothing to destroy
}

void HttpWorkerThread::operator()() {
    // This will loop while alternating between trying to grab a new socket, and sleeping if there's nothing to do.
    // The trick will be in leaving the queue unlocked enough and frequently enough for things to be added to it.
    while (server->active()) {
        {
            boost::upgrade_lock<boost::shared_mutex> lock(server->getQueueMutex());
            std::cout << "Server thread loop" << std::endl;
        }
        sleep(10);
    }
}

}