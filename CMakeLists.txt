cmake_minimum_required(VERSION 2.8)
project(minHTTP)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread")
aux_source_directory(src SRC_LIST)
add_executable(test main.cpp  ${SRC_LIST})
