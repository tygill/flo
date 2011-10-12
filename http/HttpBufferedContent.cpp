#include "http/HttpBufferedContent.h"

namespace http {

HttpBufferedContent::HttpBufferedContent() :
    code(200)
{
}

HttpBufferedContent::~HttpBufferedContent() {
}

int HttpBufferedContent::status() const {
    return code;
}

void HttpBufferedContent::setStatus(int stats) {
    code = stats;
}

}
