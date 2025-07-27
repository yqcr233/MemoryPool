#ifndef TEST_HPP
#define TEST_HPP
#include "core.hpp"
#include "simple_memory_pool.hpp"
struct node{
    int a;
    node* next;
    node(int _a): a(_a), next(nullptr){}
};

void test_simpel_memory_pool();

#endif