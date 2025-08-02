#ifndef THREAD_CACHE_HPP
#define THREAD_CACHE_HPP
#include "core.hpp"
#include "common.hpp"
#include "central_cache.hpp"
/**
 * 线程本地缓存线程池, 从central_cache中获取内存块进行分配
 */
class ThreadCache{
    public:
        static ThreadCache* getInstance(){  // 每个线程独立内存池实例
            static thread_local ThreadCache instance;
            return &instance;
        }
        void* allocate(size_t size);
        void deallocate(void* ptr, size_t size);
    private:
        void* fetchFromCentralCache(size_t index);
        void returnToCentralCache(void* start, size_t size);
        bool  shouldReturnToCentralCache(size_t index);
    private:
        array<void*, FREE_LIST_SIZE> freeList;
        array<size_t, FREE_LIST_SIZE> freeListSize;
};
#endif