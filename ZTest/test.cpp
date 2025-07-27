#include "test.hpp"

void test_simpel_memory_pool() {
    HashBucket::initMemoryPool();
    node* a = newElement<node>(1);
    cout<<a->a<<endl;
    deleteElement(a);
}