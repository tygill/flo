cmake_minimum_required(VERSION 2.8)
project(Flo)

# include_directories("$(SOME_VAR_NAME)/directory")
include_directories("${PROJECT_SOURCE_DIR}") #CMAKE_CURRENT_SOURCE_DIR
#link_directories("${PROJECT_BINARY_DIR}") #CMAKE_CURRENT_BINARY_DIR

#file(GLOB webclient_cpp
#    "*.cpp"
#    "http/*.cpp"
#)

set(webserver_cpp
    "flo.cpp"
    "http/HttpGlobal.cpp"
    "http/HttpServer.cpp"
    "http/HttpResponse.cpp"
    "http/HttpTokenizer.cpp"
    "http/HttpRequest.cpp"
    "http/HttpWorkerThread.cpp"
    "http/HttpServerThread.cpp"
    "http/HttpBufferedContent.cpp"
    "http/HttpBufferedFile.cpp"
    "http/HttpBufferedDirectory.cpp"
    "http/HttpBufferedString.cpp"
    "http/HttpBufferedCgi.cpp"
)

find_package(Boost REQUIRED COMPONENTS thread)
if (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost was not found")
endif (NOT Boost_FOUND)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

add_executable(flo ${webserver_cpp})
target_link_libraries(flo ${Boost_LIBRARIES})

file(COPY "${PROJECT_SOURCE_DIR}/www" DESTINATION "${PROJECT_BINARY_DIR}")

# install (TARGETS download DESTINATION bin)
