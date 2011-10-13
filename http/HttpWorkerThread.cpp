#include "http/HttpWorkerThread.h"
#include "http/HttpServerThread.h"
#include "http/HttpBufferedFile.h"
#include "http/HttpBufferedDirectory.h"
#include "http/HttpBufferedString.h"
#include "http/HttpBufferedCgi.h"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>

namespace http {

HttpWorkerThread::HttpWorkerThread(HttpServerThread* server) :
    serverThread(server)
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
    while (serverThread->active()) {
        int sock = -1;
        {
            boost::upgrade_lock<boost::shared_mutex> lock(serverThread->getQueueMutex());
            if (!serverThread->getQueue().empty()) {
                boost::upgrade_to_unique_lock<boost::shared_mutex> unique(lock);
                sock = serverThread->getQueue().front();
                serverThread->getQueue().pop();
                //std::cout << "Items in queue: " << serverThread->getQueue().size() << std::endl;
            }
        }
        if (sock != -1) {
            HttpRequest request;
            HttpTokenizer tokenizer(sock);
            
            // Tokenize the input stream
            //bool advanceTokenizer = true;
            bool hasContent = false;
            bool requestComplete = false;
            while (tokenizer.hasToken() && !requestComplete) {
                std::string name;
                switch (tokenizer.tokenType()) {
                case HttpTokenizer::HttpStatus:
                    // This should be an error, as a request should NOT have a status.
                    //std::cout << "Error tokenizing input (socket " << sock << "): Received HttpStatus token." << std::endl;
                break;
                case HttpTokenizer::HttpRequest:
                    //std::cout << "HttpRequest " << std::flush;
                    request.setRequest(tokenizer.tokenValue());
                break;
                case HttpTokenizer::HeaderField:
                    //std::cout << "HeaderField " << std::flush;
                    // We're going to grab the next token, and make sure it's a
                    // HeaderValue token, so store the HeaderField
                    name = tokenizer.tokenValue();
                    tokenizer.next();
                    if (tokenizer.hasToken() && tokenizer.tokenType() == HttpTokenizer::HeaderValue) {
                        request.addHeader(name, tokenizer.tokenValue());
                        // Set the content reserve size if we found the Content-Length header
                        if (name.compare("Content-Length") == 0) {
                            tokenizer.setContentReserveSize(atoi(tokenizer.tokenValue().c_str()));
                            hasContent = true;
                        }
                    } else {
                        // Print some sort of warning message - the header name
                        // has no matched pair.
                        // If we could, reset the previous token to undo the
                        // change, but at present that's not possible and probably
                        // not worth it
                        //advanceTokenizer = false;
                        continue;
                    }
                break;
                case HttpTokenizer::HeaderValue:
                    //std::cout << "HeaderValue " << std::flush;
                    // Print some sort of warning message - HeaderValues should only
                    // be found after a HeaderField
                    std::cout << "Error tokenizing input (" << sock << "): Received HeaderValue token with no name before." << std::endl;
                break;
                case HttpTokenizer::HeaderComplete:
                    //std::cout << "HeaderComplete " << std::flush;
                    if (!hasContent) {
                        requestComplete = true;
                    }
                break;
                case HttpTokenizer::Content:
                    //std::cout << "Content " << std::flush;
                    // We shouldn't ever get this for a request either (until POST is built,
                    // at any rate. So print out an error.
                    request.setParameters(tokenizer.tokenValue());
                    //std::cout << "Error tokenizing input (" << sock << "): Received Content in request." << std::endl;
                break;
                case HttpTokenizer::EndOfFile:
                    // This should never be seen here, as hasToken() doesn't count this
                    // as a token.
                break;
                case HttpTokenizer::NoConnection:
                    // No connection could be read from the socket
                    perror("Error tokenizing socket");
                break;
                }
                //if (advanceTokenizer && !requestComplete) {
                if (!requestComplete) {
                    tokenizer.next();
                }
                //}
                //advanceTokenizer = true;
            }
            
            //std::cout << "Worker received request:" << std::endl;
            //std::cout << request.toString() << std::flush;
            boost::shared_ptr<HttpResponse> response = service(&request);
            
            //std::cout << "Response:" << std::endl;
            std::string str;
            response->resetStream();
            while (!response->streamComplete()) {
                str = response->readStream(1024);
                //std::cout << str << std::flush;
                write(sock, str.c_str(), str.size());
            }
            shutdown(sock, SHUT_RDWR);
            if (close(sock) == SOCKET_ERROR) {
                perror("Error closing socket");
            }
        }
        sleep(THREAD_TIMEOUT);
    }
}

boost::shared_ptr<HttpResponse> HttpWorkerThread::service(HttpRequest* request) const {
    switch (request->getType()) {
    case HTTP_GET:
    case HTTP_POST:
        return get(request);
    break;
    default:
        return error(405);
    break;
    }
}

boost::shared_ptr<HttpResponse> HttpWorkerThread::get(HttpRequest* request) const {
    // Sanitize the uri (remove ../ directories - we don't want requests able to search our
    // whole computer)
    // In thinking about this, there's probably a better standard sanitize function somewhere.
    // Oh well for now...this works. Look into that for later.
    // Yep, boost has a filesystem library. Next time, I'll have to use that.
    std::string uri(request->getUri());
    size_t find = uri.find("../");
    while (find != std::string::npos) {
        uri.replace(find, 3, "/");
        find = uri.find("../");
    }

    std::string path(serverThread->getRoot());
    path.append(uri);
    
    // Is path a directory or a file?
    struct stat filestat;
    if (stat(path.c_str(), &filestat) == 0) {
        // This way, getFile and getDirectory can work under the assumption that the
        // file does exist.
        if (S_ISREG(filestat.st_mode)) {
            return getFile(uri, request);
        } else if (S_ISDIR(filestat.st_mode)) {
            return getDirectory(uri, request);
        } else {
            return error(404);
        }
    } else {
        return error(404);
    }
}

boost::shared_ptr<HttpResponse> HttpWorkerThread::getFile(const std::string& uri, HttpRequest* request) const {
    std::string path(serverThread->getRoot());
    path.append(uri);
    
    struct stat filestat;
    if (stat(path.c_str(), &filestat) == 0) {
        
        //std::cout << request->getExt() << std::endl;
        /*
        // Decide what type of file was requested (cgi, php-cgi, or raw)
        size_t pos = path.find_last_of(".");
        std::string ext;
        if (pos != std::string::npos) {
            try {
                ext = path.substr(pos+1);
            } catch (std::out_of_range& e) {
                // There is a . in the path, but it is the last character.
                ext = "";
            }
        } //else there is no extension, so the default value of "" is ok.
        */
        // Check the type of file (.php, .cgi, .pl, other)
        if (request->getExt().compare("cgi") == 0 || request->getExt().compare("pl") == 0 || request->getExt().compare("php") == 0) {
            // CGI script
            
            boost::shared_ptr<HttpBufferedContent> content(new HttpBufferedCgi(path, request));
            boost::shared_ptr<HttpResponse> response(new HttpResponse());
            response->setVersion(HTTP_1_1);
            response->setStatusCode(200);
            response->setContent(content);
            return response;
        } else {
            // Normal file
            
            // Print this only for raw file data (dynamic scripts should send this on their own)
            boost::shared_ptr<HttpBufferedContent> content(new HttpBufferedFile(path, filestat.st_size));
            if (content->status() < 400) {
                boost::shared_ptr<HttpResponse> response(new HttpResponse());
                response->setVersion(HTTP_1_1);
                response->setStatusCode(200);
                response->addHeader("Content-Type", mimeType(request->getExt()));
                response->setContent(content);
                return response;
            } else {
                return error(content->status());
            }
        }
    } else {
        return error(404);
    }
}

boost::shared_ptr<HttpResponse> HttpWorkerThread::getDirectory(const std::string& uri, HttpRequest* request) const {
    // Make sure the last character of the uri is a '/'
    std::string cleanUri(uri);
    try {
        if (cleanUri.at(cleanUri.size()-1) != '/') {
            cleanUri.push_back('/');
            // I'm tempted to force a redirect on the client when they get to this point...
            // I won't do it yet, but eventually I might. That way, sub gets linked as sub/
        }
    } catch (std::out_of_range& e) {
        std::cerr << "Warning: Reading contents of root filesystem directory!" << std::endl;
        cleanUri = "/";
    }
    
    std::string path(serverThread->getRoot());
    path.append(cleanUri);
    
    // First, check for index.html & index.php
    std::string html(path);
    html.append("index.html");
    if (access(html.c_str(), F_OK) == 0) {
        std::string htmlUri(cleanUri);
        htmlUri.append("index.html");
        request->setUri(htmlUri);
        return get(request);
    }
    std::string php(path);
    php.append("index.php");
    if (access(php.c_str(), F_OK) == 0) {
        std::string phpUri(cleanUri);
        phpUri.append("index.php");
        request->setUri(phpUri);
        return get(request);
    }
    
    boost::shared_ptr<HttpBufferedDirectory> content(new HttpBufferedDirectory(path, cleanUri));
    if (content->status() < 400) {
        boost::shared_ptr<HttpResponse> response(new HttpResponse());
        response->setVersion(HTTP_1_1);
        response->setStatusCode(200);
        response->setContent(content);
        return response;
    } else {
        return error(content->status());
    }
    
    
    /*
    // If neither of those files existed, then we read the directory and build a little html
    // listing of everything
    DIR* dir;
    struct dirent* ent;
    dir = opendir(path.c_str());
    if (dir != NULL) {
        boost::shared_ptr<HttpResponse> response(new HttpResponse());
        response->setVersion(HTTP_1_1);
        response->setStatusCode(200);
        std::string content("<html>\n<head>\n<title>Directory Listing</title>\n</head>\n<body>\n");
        
        //int len = serverThread->getRoot().size();
        while ((ent = readdir(dir)) != NULL) {
            // Hard coded optimization for the reserve size for these loops - otherwise it was
            // taking quite a while. This is still far from efficient, but its easy and it
            // works.
            content.reserve(content.size() + 13 + cleanUri.size() + (2 * strlen(ent->d_name)) + 2 + 10);
            //printf ("%s\n", ent->d_name);
            content.append("<p>\n<a href=\"");
            content.append(cleanUri.c_str());
            content.append(ent->d_name);
            content.append("\">");
            content.append(ent->d_name);
            content.append("</a>\n</p>\n");
        }
        closedir(dir);
        content.append("</body>\n</html>");
        response->setContent(content);
        return response;
    } else {
        perror("Could not open directory");
        return error(404);
    }
    */
}

boost::shared_ptr<HttpResponse> HttpWorkerThread::error(int statusCode) const {
    boost::shared_ptr<HttpResponse> response(new HttpResponse());
    response->setVersion(HTTP_1_1);
    response->setStatusCode(statusCode);
    std::string content;
    content.append("<html>\n<head>\n<title>");
    content.append(statusCodeToString(statusCode));
    content.append("</title>\n</head>\n<body>\n<h1>");
    std::stringstream code;
    code << statusCode;
    content.append(code.str());
    content.append(" Error: ");
    content.append(statusCodeToString(statusCode));
    content.append("</h1>\n</body>\n</html>");
    boost::shared_ptr<HttpBufferedContent> contentBuffer(new HttpBufferedString(content));
    response->setContent(contentBuffer);
    return response;
}

}