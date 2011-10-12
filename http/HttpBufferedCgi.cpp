#include "http/HttpBufferedCgi.h"

#include <http/HttpBufferedString.h>

#include <iostream>
#include <cstring>
#include <cctype>

namespace http {

HttpBufferedCgi::HttpBufferedCgi(const std::string& p, HttpRequest* req) :
    request(req),
    path(p),
    initialized(false),
    complete(false)
{
    resetStream();
}

HttpBufferedCgi::~HttpBufferedCgi() {
    close(pipeTo[1]);
    close(pipeFrom[0]);
}

void HttpBufferedCgi::resetStream() {
    if (initialized) {
        close(pipeTo[1]);
        close(pipeFrom[0]);
        initialized = true;
    }
    pipe(pipeTo);
    pipe(pipeFrom);
    
    bool isPhp = request->getExt().compare("php") == 0;
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child (exec to pipe)
        close(pipeTo[1]);
        dup2(pipeTo[0], 0);
        close(pipeFrom[0]);
        dup2(pipeFrom[1], 1);
        
        // Set up argv
        char** argv = new char*[2];
        argv[0] = new char[path.size()+1];
        strncpy(argv[0], path.c_str(), path.size());
        argv[path.size()] = '\0';
        argv[1] = NULL;
        
        // Set up env
        // Prepend http
        std::map<std::string, std::string> cache = request->getHeaders();
        for (std::map<std::string, std::string>::iterator itr = cache.begin(); itr != cache.end(); itr++) {
            std::string newName;
            if (itr->first.compare("Content-Length") != 0 && itr->first.compare("Content-Type") != 0) {
                newName.append("Http-");
                request->removeHeader(itr->first);
            }
            newName.append(itr->first);
            request->addHeader(newName, itr->second);
        }
        if (isPhp) {
            request->addHeader("Script-Filename", path);
            request->addHeader("Script-Name", request->getUri());
            request->addHeader("Redirect-Status", "200");
        }
        // 4 custom vars:
        /*
        GATEWAY_INTERFACE=CGI/1.1
        REQUEST_URI=%s where %s is the url
        REQUEST_METHOD=%s where %s is either POST or GET
        QUERY_STRING=%s where %s is everything after ? in the url
        */
        request->addHeader("Gateway-Interface", "CGI/1.1");
        request->addHeader("Request-Uri", request->getUri());
        if (request->getType() == HTTP_POST) {
            request->addHeader("Request-Method", "POST");
        } else if (request->getType() == HTTP_GET) {
            request->addHeader("Request-Method", "GET");
            request->addHeader("Query-String", request->getParameterString());
        }
        
        //std::cout << "Environment:" << std::endl;
        char** env;
        int envCount = request->getHeaders().size();
        env = new char*[envCount+1];
        env[envCount] = NULL;
        int envIndex = 0;
        for (std::map<std::string, std::string>::const_iterator itr = request->getHeaders().begin(); itr != request->getHeaders().end(); itr++) {
            std::string var(itr->first);
            for (unsigned int i = 0; i < var.size(); i++) {
                if (var[i] != '-') {
                    var[i] = toupper(var[i]);
                } else {
                    var[i] = '_';
                }
            }
            var.push_back('=');
            var.append(itr->second);
            env[envIndex] = new char[var.size()+1];
            env[envIndex][var.size()] = '\0';
            strncpy(env[envIndex], var.c_str(), var.size());
            envIndex++;
            //std::cout << var << std::endl;
        }
        
        //std::cout << "Request:" << std::endl;
        //std::string str;
        //request->resetStream();
        //while (!request->streamComplete()) {
            //str = request->readStream(1024);
            //std::cout << str << std::flush;
        //}
        
        //std::cout << "Exec..." << std::endl;
        if (isPhp) {
            execve("/usr/bin/php-cgi", argv, env);
        } else {
            execve(path.c_str(), argv, env);
        }
        // The thread will die, so nothing needs deletion!
    } else {
        // Parent (read from pipe)
        close(pipeTo[0]);
        close(pipeFrom[1]);
        
        if (request->getType() == HTTP_POST) {
            // Write the post string to the cgi script
            //std::cout << "Writing to pipe" << std::endl;
            //std::cout << request->getParameterString() << std::endl;
            write(pipeTo[1], request->getParameterString().c_str(), request->getParameterString().size());
            write(pipeTo[1], "\r\n", 2);
        }
    }
}

bool HttpBufferedCgi::streamComplete() const {
    return complete;
}

std::string HttpBufferedCgi::readStream(int size) {
    std::string buffer;
    buffer.reserve(size);
    char c;
    int amtRead;
    for (int i = 0; i < size && !complete; i++) {
        amtRead = read(pipeFrom[0], &c, 1);
        if (amtRead != 0) {
            buffer.push_back(c);
        } else {
            complete = true;
        }
    }
    return buffer;
}

}
