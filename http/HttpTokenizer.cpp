
#include <sys/types.h>
#include <sys/socket.h>
//#include <netdb.h>

#include <cctype>

#include <iostream>
#include <cstdio>

#include "http/HttpTokenizer.h"
#include "http/HttpGlobal.h"

namespace http {

HttpTokenizer::HttpTokenizer(int socketHandle) :
    sock(socketHandle),
    currTokenType(EndOfFile),
    headerComplete(false),
    contentReserveSize(0)
{
    next(true);
}

HttpTokenizer::~HttpTokenizer() {
    // DON'T close the socket - this way, we can keep the connection open and respond.
    // Close the socket up
    //if (close(sock) == SOCKET_ERROR) {
        // Print error message, but by now we have what we wanted anyway,
        // so we won't return an error message...usually closing shouldn't
        // have problems anyway, I'd hope.
        //perror("Error closing socket");
    //}
}

bool HttpTokenizer::hasToken() const {
    return currTokenType != EndOfFile && currTokenType != NoConnection;
}

HttpTokenizer::TokenType HttpTokenizer::tokenType() const {
    return currTokenType;
}

std::string HttpTokenizer::tokenValue() const {
    return currTokenValue;
}

void HttpTokenizer::next() {
    next(false);
}

void HttpTokenizer::setContentReserveSize(int size) {
    contentReserveSize = size;
}

void HttpTokenizer::next(bool firstTime) {
    unsigned char current = '\0';
    int readLen = 0;
    if (!firstTime) {
        switch (currTokenType) {
        case HttpStatus:
        case HttpRequest:
        case HeaderValue:
            // In these cases we'll be looking for another HeaderName (':')
            // or HeaderComplete ('[\r]\n[\r]\n')
            while (buffer.find('\n') == std::string::npos && buffer.find(':') == std::string::npos && (readLen = read(sock, &current, 1)) > 0) {
                buffer.push_back(current);
            }
            if (readLen >= 0) {
                if (buffer.at(buffer.length() - 1) == ':') { //buffer.find(':') != buffer.end()
                    // Remove the trailing :
                    buffer.erase(buffer.length() - 1);
                    currTokenType = HeaderField;
                    currTokenValue = trim(buffer);
                    buffer = "";
                } else if (buffer.at(buffer.length() - 1) == '\n') {
                    // We have an end of line, which either means we have the end
                    // of the header, or an invalid line.
                    // Ending of headers should only be found when there is a
                    // completely empty line, but we won't check to make sure the
                    // lines completely empty. We'll just check to make sure theres
                    // no :, and then call it the end
                    currTokenType = HeaderComplete;
                    currTokenValue = trim(buffer);
                    buffer = "";
                } else {
                    // Handle the third case caused by a read of 0
                    currTokenType = EndOfFile;
                    currTokenValue = buffer;
                    buffer = "";
                }
            } else {
                currTokenType = NoConnection;
                currTokenValue = buffer;
                buffer = "";
            }
        break;
        case HeaderField:
            // Look for a HeaderValue
            while (buffer.find('\n') == std::string::npos && (readLen = read(sock, &current, 1)) > 0) {
                buffer.push_back(current);
            }
            if (readLen >= 0) {
                if (buffer.at(buffer.length() - 1) == '\n') {
                    // Header value found
                    currTokenType = HeaderValue;
                    currTokenValue = trim(buffer);
                    buffer = "";
                } else {
                    // Handle the read of size 0 case
                    currTokenType = EndOfFile;
                    currTokenValue = buffer;
                    buffer = "";
                }
            } else {
                currTokenType = NoConnection;
                currTokenValue = buffer;
                buffer = "";
            }
        break;
        case HeaderComplete:
            // Everything else we can read is the content
            while ((readLen = read(sock, &current, 1)) > 0) {
                buffer.push_back(current);
            }
            if (readLen >= 0) {
                // If there's something in the buffer, we have content. Otherwise,
                // there's nothing.
                if (!buffer.empty()) {
                    currTokenType = Content;
                    // Don't trim the content, we want extra whitespace before and after it.
                    currTokenValue = buffer;
                    buffer = "";
                } else {
                    currTokenType = EndOfFile;
                    currTokenValue = "";
                    buffer = "";
                }
            } else {
                currTokenType = NoConnection;
                currTokenValue = buffer;
                buffer = "";
            }
        break;
        case Content:
        default:
            currTokenType = EndOfFile;
            currTokenValue = "";
            buffer = "";
        break;
        case EndOfFile:
        case NoConnection:
            // Do nothing. No connection to do anything with.
        break;
        }
    } else {
        // On the first time, the token we need to get always going to be the
        // HttpStatus one, so we just read until we get a \n, then trim out
        // the whitespace
        // We don't need to worry about scanning the previously loaded buffer
        // for \n
        buffer.reserve(contentReserveSize);
        while (current != '\n' && (readLen = read(sock, &current, 1)) > 0) {
            buffer.push_back(current);
        }
        if (readLen >= 0) {
            // In both these two cases, the token receiving class is responsible for parsing out the
            // exact detains of the pieces of these parts.
            if (buffer.find("HTTP") == 0) {
                // This means it was a response that was given (responses begin with HTTP/1.x), so
                // this first line is a status.
                currTokenType = HttpStatus;
                currTokenValue = trim(buffer);
                buffer = "";
            } else {
                // This means it was a request that is being parsed (requests begin with GET, POST,
                // etc.), so the first line is a request.
                currTokenType = HttpRequest;
                currTokenValue = trim(buffer);
                buffer = "";
            }
        } else {
            currTokenType = NoConnection;
            currTokenValue = "";
            buffer = "";
        }
    }
}

}
