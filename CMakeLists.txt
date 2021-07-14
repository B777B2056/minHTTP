cmake_minimum_required(VERSION 2.8)
project(minHTTP)
aux_source_directory(src SRC_LIST)
add_executable(client_test main.cpp  ${SRC_LIST})
