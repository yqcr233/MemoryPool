cmake_minimum_required(VERSION 3.15)
project(FastMemoryPool)

add_library(fast_memory_pool fast_memory_pool.cpp)

target_include_directories(fast_memory_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)