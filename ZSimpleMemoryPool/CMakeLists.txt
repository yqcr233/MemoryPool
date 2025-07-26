cmake_minimum_required(VERSION 3.15)
project(SimpleMemoryPool)

add_library(simple_memory_pool simple_memory_pool.cpp)

target_include_directories(simple_memory_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)