cmake_minimum_required(VERSION 3.10)
project(SocketServer)

# Set the C++ standard (you can use CXX if you're using C++)
set(CMAKE_C_STANDARD 11)

# BASIC VERSION

# Define the source files for the server and client
set(SERVER_SOURCE socket_server.c)
set(CLIENT_SOURCE socket_client.c)

# Add executable for the server
add_executable(socket_server ${SERVER_SOURCE})

# Add executable for the client
add_executable(socket_client ${CLIENT_SOURCE})

# SIMPLE FILE TRANSFER

# Define the source files for the server and client
set(SERVER_SOURCE_FILE file_server.c)
set(CLIENT_SOURCE_FILE file_client.c)

# Add executable for the server
add_executable(file_server ${SERVER_SOURCE_FILE})

# Add executable for the client
add_executable(file_client ${CLIENT_SOURCE_FILE})