cmake_minimum_required(VERSION 3.15)
project(Test)

add_library(test test.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test PRIVATE simple_memory_pool three_level_memorypool)