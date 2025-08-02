#ifndef TEST_HPP
#define TEST_HPP
#include "core.hpp"
#include "simple_memory_pool.hpp"
#include "thread_cache.hpp"
#include "central_cache.hpp"
struct node{
    int a;
    node* next;
    node(int _a): a(_a), next(nullptr){}
};

void test_simpel_memory_pool();

void test_three_level_memory_pool();

#endif