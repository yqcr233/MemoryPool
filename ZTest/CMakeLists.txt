cmake_minimum_required(VERSION 3.15)
project(Test)

add_library(test test.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)