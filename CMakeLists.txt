cmake_minimum_required(VERSION 3.15)
project(MemoryPool)

set(SRC_LIST main.cpp)

add_executable(memory_pool ${SRC_LIST})

add_subdirectory(ZSimpleMemoryPool)
add_subdirectory(ZTest)

target_include_directories(memory_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(memory_pool PRIVATE simple_memory_pool test)

