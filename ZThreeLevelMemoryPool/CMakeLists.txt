cmake_minimum_required(VERSION 3.15)
project(three_level_memorypool)

add_library(three_level_memorypool thread_cache.cpp page_cache.cpp central_cache.cpp)

target_include_directories(three_level_memorypool PRIVATE ${CMAKE_SOURCE_DIR}/include)