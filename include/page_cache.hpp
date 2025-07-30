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

};
#endif