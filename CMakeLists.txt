cmake_minimum_required(VERSION 3.22.3)
project(prog)
add_executable(prog src/main.cpp)
target_include_directories(prog PRIVATE inc)