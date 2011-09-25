#include "flo.h"

#include <cstdlib>
#include <cctype>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

#include "http/HttpServer.h"

int main(int argc, char* argv[]) {
    http::HttpServer server("www", 8888);
    if (server.start()) {
        while (true) {
            sleep(1);
        }
    }
    return 0;
}