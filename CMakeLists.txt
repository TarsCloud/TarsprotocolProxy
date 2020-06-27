cmake_minimum_required(VERSION 2.8)

project(protocolproxy-server)

option(TAF_MYSQL "option for mysql" OFF)
option(TAF_SSL "option for ssl" OFF)
option(TAF_HTTP2 "option for http2" OFF)
option(TAF_GPREF "option for gpref" OFF)

if(WIN32)
    include (c:\\tars\\cpp\\makefile\\tars-tools.cmake)
else()
    include (/usr/local/tars/cpp/makefile/tars-tools.cmake)
endif()

set(TAF_WEB_HOST "172.16.8.227:3000")

include_directories(/usr/local/tars/cpp/thirdparty/include)
link_directories(/usr/local/tars/cpp/thirdparty/lib)

add_subdirectory(src)

set(WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

