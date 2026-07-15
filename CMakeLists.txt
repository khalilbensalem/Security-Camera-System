cmake_minimum_required(VERSION 3.22.3)
project(prog)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(prog src/main.cpp src/TcpClient.cpp)
target_include_directories(prog PRIVATE inc)