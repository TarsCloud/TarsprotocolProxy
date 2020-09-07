cmake_minimum_required(VERSION 2.8)

project(protocolproxy-server)

option(TARS_MYSQL "option for mysql" OFF)
option(TARS_SSL "option for ssl" OFF)
option(TARS_HTTP2 "option for http2" OFF)
option(TARS_GPREF "option for gpref" OFF)

if(WIN32)
    include (c:\\tars\\cpp\\makefile\\tars-tools.cmake)
else()
    include (/usr/local/tars/cpp/makefile/tars-tools.cmake)
endif()

####you can: cd build; cmake .. -DTARS_WEB_HOST={your web host}
set(TARS_WEB_HOST "" CACHE STRING "set web host")
IF (TARS_WEB_HOST STREQUAL "")
        set(TARS_WEB_HOST "http://tars.test.com")
ENDIF ()

include_directories(/usr/local/tars/cpp/thirdparty/include)
link_directories(/usr/local/tars/cpp/thirdparty/lib)

add_subdirectory(src)

set(WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

