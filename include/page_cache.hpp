#ifndef PAGE_CACHE_HPP
#define PAGE_CACHE_HPP
#include "core.hpp"
class PageCache{
    public:
        static const size_t PAGE_SIZE = 4096; // 每页大小4k
        static PageCache& getInstance() {
            static PageCache instance;
            return instance;
        }
        void* allocateSpan(size_t numPages);
        void deallocateSpan(void* ptr, size_t numPages);
    private:
        PageCache() = default;
        void* systemAlloc(size_t numPages);
    private:
        struct Span{
            void* pageAddr; // 页起始地址
            size_t numPages;
            Span* next;
        };  
        map<size_t, Span*> freeSpans; // size_t代表页数，这里按页数管理不同空闲链表
        map<void*, Span*> spanMap;   // 维护地址到span的映射，便于合并操作
        std::mutex mutex;
};
#endif