cmake_minimum_required(VERSION 3.8)
project(dehaze)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(dehaze ${SOURCE_FILES})