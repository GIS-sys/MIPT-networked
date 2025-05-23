cmake_minimum_required(VERSION 3.10)  # Minimum CMake version required

project(networked_w1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_SOURCES 
    socket_tools.cpp
    common.cpp
)

add_executable(server
    server.cpp
    ${SHARED_SOURCES}
)

add_executable(client
    client.cpp
    ${SHARED_SOURCES}
)
