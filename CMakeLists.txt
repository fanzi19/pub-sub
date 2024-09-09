cmake_minimum_required(VERSION 3.27)
project(pub_sub)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR})

# Server executable
add_executable(server
        broker/main.cpp
        broker/server.cpp
        common/message.cpp
        common/network.cpp
)

# Publisher client executable
add_executable(publisher_client
        client/publisher_client.cpp
        client_api/publisher.cpp
        common/message.cpp
        common/network.cpp
)

# Subscriber client executable
add_executable(subscriber_client
        client/subscriber_client.cpp
        client_api/subscriber.cpp
        common/message.cpp
        common/network.cpp
)

# Find and link against pthread
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
target_link_libraries(publisher_client Threads::Threads)
target_link_libraries(subscriber_client Threads::Threads)

# Add socket programming flags
target_compile_definitions(server PRIVATE _GNU_SOURCE)
target_compile_definitions(publisher_client PRIVATE _GNU_SOURCE)
target_compile_definitions(subscriber_client PRIVATE _GNU_SOURCE)