#ifndef COMMON_HPP
#define COMMON_HPP
#include "core.hpp"
// 对齐数和大小定义
constexpr size_t ALIGNMENT = 8;
constexpr size_t MAX_BYTES = 256 * 1024; // 256KB
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT; // ALIGNMENT等于指针void*的大小
constexpr size_t THRESHOULD = 256; // 线程本地缓存单自由链表长度限制

// 内存块信息
struct BLockHeader{
    size_t size; // 该内存块大小
    bool isUse;  // 使用标志
    BLockHeader* next;
};

/**
 * 块管理
 */
class SizeClass{
    public:
        static size_t roundUP(size_t size){ // 内存对齐到ALIGNMENT
            return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1); // & ~(ALIGNMENT - 1) 抹去大于ALIGNMENT倍数的部分
        }

        static size_t getIndex(size_t size) { // 根据所需内存大小分配合适的内存块
            size = max((size_t)1, size);
            return (size + ALIGNMENT - 1) / ALIGNMENT;
        }   
};
#endif