#include "central_cache.hpp"

void* CentralCache::fetchFromPageCache(size_t size){
    // 计算所需的页数
    size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
    if(numPages <= SPAN_PAGE) { // 所需内存小于8页时申请8页
        return PageCache::getInstance().allocateSpan(SPAN_PAGE);
    }
    else{ // 所需内存大于8页时按需分配
        return PageCache::getInstance().allocateSpan(numPages);
    }
}
SpanTracker* CentralCache::getSpanTracker(void* blockAddr){
    for(int i = 0; i < spanCount.load(); i++) {
        void* spanAddr = spanTracker[i].spanAddr.load();
        size_t numPages = spanTracker[i].numPages.load();
        // static_cast<char*>(spanAddr), spanAddr是void*类型不能直接进行算术计算，所以转成char*
        // 在C++中，指针运算的步长取决于指针所指向的类型，所以转换成char*所以能正确进行指针运算
        if(blockAddr >= spanAddr && blockAddr < static_cast<char*>(spanAddr) + numPages * PageCache::PAGE_SIZE) {
            return &spanTracker[i];
        }
    }
    return nullptr;
}
void CentralCache::updateSpanFreeCount(SpanTracker* tracker, size_t newFreeBlock, size_t index){
    tracker->freeCount.store(newFreeBlock);
    if(newFreeBlock == tracker->blockCount.load()) {
        void* spanAddr = tracker->spanAddr.load();
        size_t numPages = tracker->numPages.load();
        void* current = centralFreeList[index].load();
        void* newHead = nullptr;
        void* prev = nullptr;
        // span中所有block都已空闲则释放
        while(current) {
            void* next = *reinterpret_cast<void**>(current);
            // 从中心缓存空闲链表中删除归还的block
            if(current >= spanAddr && current < reinterpret_cast<char*>(spanAddr) + numPages * PageCache::PAGE_SIZE) {
                if(prev) {
                    *reinterpret_cast<void**>(prev) = next;
                }else{
                    newHead = next;
                }
            }else{
                prev = current;
            }
            current = next;
        }
        centralFreeList[index].store(newHead);
        PageCache::getInstance().deallocateSpan(spanAddr, numPages);
    }   
}
bool CentralCache::shouldPerformDelayReturn(size_t index, size_t currentCount, chrono::steady_clock::time_point currentTime){
    // 基于计数和时间的双重检查
    if(currentCount >= MAX_DELAY_COUNT) {
        return true;
    }
    auto lastTime = lastReturnTimes[index];
    return (currentTime - lastTime) >= DELAY_INTERVAL;
}
void CentralCache::PerformDelayReturn(size_t index){
    // 重置延迟数据
    delay_count[index].store(0);
    lastReturnTimes[index] = chrono::steady_clock::now();
    // 通过该自由链表下每个span中的空闲block数
    unordered_map<SpanTracker*, size_t> spanFreeCounts;
    void* currentBLock = centralFreeList[index].load();
    while (currentBLock)
    {
        SpanTracker* tracker = getSpanTracker(currentBLock); // 查询该空闲内存块所在span
        if(tracker) {
            spanFreeCounts[tracker]++;
        }
        currentBLock = *reinterpret_cast<void**>(currentBLock);
    }
    for(const auto& [track, newFreeBlocks]: spanFreeCounts) {
        updateSpanFreeCount(track, newFreeBlocks, index);
    }
}
void* CentralCache::fetchRange(size_t index){
    if(index >= FREE_LIST_SIZE) { // 所需内存过大，向系统直接申请
        return nullptr;
    }
    // test_and_set 自旋锁尝试获取锁，成功返回true，失败返回false
    while(locks[index].test_and_set()) {
        this_thread::yield(); // 让出cpu
    }
    void* result = nullptr;
    try{
        result = centralFreeList[index].load(); // 尝试直接从中心缓存获取
        if(!result) { // 中心无空闲块
            // 从页缓存获取新空闲块
            size_t size = (index + 1) * ALIGNMENT; // index是从0开始记录所以这里加1
            result = fetchFromPageCache(size);
            if(!result) { 
                // 从页缓存获取失败则释放锁并返回
                locks[index].clear();
                return nullptr;
            }
            // 计算实际分配的内存页数，这里size只是一个block的大小所以一定size <= SPAN_PAGE * PageCache::PAGE_SIZE
            size_t numPages = (size <= SPAN_PAGE * PageCache::PAGE_SIZE) ? SPAN_PAGE : (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
            // 根据页数计算实际块数
            size_t blockNum = (numPages * PageCache::PAGE_SIZE) / size;
            if(blockNum > 1) {
                // 将获取的页分成块并构建链表
                char* start = static_cast<char*>(result);
                for(size_t i = 1; i < blockNum; i++) {
                    void* current = start + (i - 1) * size;
                    void* next = start + i * size;
                    *reinterpret_cast<void**>(current) = next;
                }
                *reinterpret_cast<void**>(start + (blockNum - 1) * size) = nullptr;
                // 将第一个块与链表断开作为返回值
                void* next = *reinterpret_cast<void**>(result);
                *reinterpret_cast<void**>(result) = nullptr;
                centralFreeList[index].store(next);
                // 记录spantracker
                size_t trackerIndex = spanCount ++;
                if(trackerIndex < spanTracker.size()) {
                    spanTracker[trackerIndex].spanAddr.store(start);
                    spanTracker[trackerIndex].numPages.store(numPages);
                    spanTracker[trackerIndex].blockCount.store(blockNum);
                    spanTracker[trackerIndex].freeCount.store(blockNum - 1);
                }
            }
        }else{ // 中心缓存内存池中仍有多余内存块
            void* next = *reinterpret_cast<void**>(result);
            *reinterpret_cast<void**>(result) = nullptr;
            centralFreeList[index].store(next);
            // 更新该内存块所在span的空闲块计数
            SpanTracker* tracker = getSpanTracker(result);
            if(tracker) {
                tracker->freeCount.fetch_sub(1);
            }
        }
    }catch(...) {
        locks[index].clear();
        throw;
    }
    locks[index].clear();
    return nullptr;
}
void CentralCache::returnRange(void* start, size_t size, size_t index){
    if(!start || index >= FREE_LIST_SIZE) {
        return;
    }
    size_t blockSize = (index + 1) * ALIGNMENT;
    size_t blockCount = size / blockSize;
    while(locks[index].test_and_set()) {
        this_thread::yield();
    }
    try{
        void* end = start;
        size_t count = 1;
        while(*reinterpret_cast<void**>(end) != nullptr && count < blockCount) {
            end = *reinterpret_cast<void**>(end);
            count ++;
        }
        void* current = centralFreeList[index].load();
        *reinterpret_cast<void**>(end) = current; // 头插法将归还的空闲块连接到中央缓存空闲链表
        centralFreeList[index].store(start);
        // spantracker里的freeCount统计只是在归还到页缓存中才需要所以归还时统计即可不用在这里统计，提高性能
        // 延迟计数和延迟归还
        size_t currentCount = delay_count[index].fetch_add(1);
        auto currentTime = chrono::steady_clock::now();
        if(shouldPerformDelayReturn(index, currentCount, currentTime)) {
            PerformDelayReturn(index);
        }
    }catch(...) {
        locks[index].clear();
        throw;
    }
    locks[index].clear();
}