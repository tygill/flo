#include "http/HttpBufferedDirectory.h"

#include <sstream>
#include <dirent.h>
#include <cstring>
#include <cstdio>

namespace http {

HttpBufferedDirectory::HttpBufferedDirectory(const std::string& path, const std::string& uri) :
    HttpBufferedString("")
{
    DIR* dir;
    struct dirent* ent;
    dir = opendir(path.c_str());
    if (dir != NULL) {
        std::string buff("<html>\n<head>\n<title>Directory Listing</title>\n</head>\n<body>\n"); // 62
        while ((ent = readdir(dir)) != NULL) {
            // Hard coded optimization for the reserve size for these loops - otherwise it was
            // taking quite a while. This is still far from efficient, but its easy and it
            // works.
            buff.reserve(buff.size() + 13 + uri.size() + (2 * strlen(ent->d_name)) + 2 + 10);
            //printf ("%s\n", ent->d_name);
            buff.append("<p>\n<a href=\"");
            buff.append(uri.c_str());
            buff.append(ent->d_name);
            buff.append("\">");
            buff.append(ent->d_name);
            buff.append("</a>\n</p>\n");
        }
        closedir(dir);
        buff.append("</body>\n</html>");
        
        std::string header("Content-Type: text/html\r\nContent-Length: ");
        std::stringstream size;
        size << buff.size();
        header.append(size.str());
        header.append("\r\n\r\n");
        header.append(buff);
        
        setBuffer(header);
        setStatus(200);
    } else {
        setStatus(404);
    }
}

HttpBufferedDirectory::~HttpBufferedDirectory() {
}

}
