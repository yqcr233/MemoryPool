#ifndef central_cache_HPP
#define central_cache_HPP
#include "core.hpp"
#include "common.hpp"
#include "page_cache.hpp"
/**
 *  无锁span信息存储
 */
struct SpanTracker{
    atomic<void*> spanAddr{nullptr};
    atomic<size_t> numPages{0};
    atomic<size_t> blockCount{0};
    atomic<size_t> freeCount{0};
};
// 每次从PageCache至少获取span大小
constexpr size_t SPAN_PAGE = 8;
class CentralCache{
    public:
        static CentralCache& getInstance(){
            static CentralCache instance;
            return instance;
        }
        void* fetchRange(size_t index);
        void returnRange(void* start, size_t size, size_t index);
    private:
        CentralCache() = default;
        void* fetchFromPageCache(size_t size);
        SpanTracker* getSpanTracker(void* blockAddr);
        void updateSpanFreeCount(SpanTracker* tracker, size_t newFreeBlock, size_t index);
        bool shouldPerformDelayReturn(size_t index, size_t currentCount, chrono::steady_clock::time_point currentTime);
        void PerformDelayReturn(size_t index);
    private:    
        array<atomic<void*>, FREE_LIST_SIZE> centralFreeList;
        array<atomic_flag, FREE_LIST_SIZE> locks;
        array<SpanTracker, 1024> spanTracker;
        atomic<size_t> spanCount{0};
        // 延迟归还相关变量
        static const size_t MAX_DELAY_COUNT = 48; // 最大延迟计数
        array<atomic<size_t>, FREE_LIST_SIZE> delay_count; // 每个自由链表的延迟计数
        array<chrono::steady_clock::time_point, FREE_LIST_SIZE> lastReturnTimes; // 上次归还时间
        static const chrono::milliseconds DELAY_INTERVAL; // 延迟间隔
};
#endif