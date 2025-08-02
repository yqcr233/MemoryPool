#include "page_cache.hpp"

void* PageCache::allocateSpan(size_t numPages){
    lock_guard<std::mutex> lock(mutex);
    // 查找合适大小的空闲块
    auto it = freeSpans.find(numPages);
    if (it != freeSpans.end()) {
        Span* span = it->second;
        if(span->next) {
            freeSpans[it->first] = span->next;
        }
        else {
            freeSpans.erase(it);
        }
        // 如果span过大则进行分割
        if(span->numPages > numPages) {
            Span* new_span = new Span();
            new_span->pageAddr = reinterpret_cast<char*>(span->pageAddr) + numPages * PAGE_SIZE;
            new_span->numPages = span->numPages - numPages;
            new_span->next = nullptr;
            span->numPages = numPages;
            // 将超出部分重新放入空闲链表
            auto& list = freeSpans[new_span->numPages];
            new_span->next = list;
            list = new_span;
        }
        // 更新span信息便于回收
        spanMap[span->pageAddr] = span;
        return span->pageAddr;
    }
    // 没有合适页缓存则向系统申请
    void* memory = systemAlloc(numPages);
    if(memory == nullptr) {
        return nullptr;
    }
    Span* span = new Span();
    span->pageAddr = memory;
    span->numPages = numPages;
    span->next = nullptr;
    // 记录审判信息用于回收
    spanMap[memory] = span;
    return memory;
}
void PageCache::deallocateSpan(void* ptr, size_t numPages){
    lock_guard<std::mutex> lock(mutex);
    auto it = spanMap.find(ptr);
    if(it == spanMap.end()) { // 不是页缓存分配的直接返回
        return;
    }
    Span* span = it->second;
    // 尝试合并相邻内存块
    void* nextAddr = static_cast<char*>(ptr) + numPages * PAGE_SIZE;
    auto nextIt = spanMap.find(nextAddr);
    if(nextIt != spanMap.end()) {
        Span* nextSpan = nextIt->second;
        // 检查nextSpan是否空闲，空闲时才合并
        bool found = false;
        auto &list = freeSpans[nextSpan->numPages];
        if(list == nextSpan) { // 该节点存在且是头节点
            found = true;
            list = list->next;
        } else{
            Span* prev = list;
            while(prev->next) {
                if(prev->next == nextSpan) {
                    prev->next = nextSpan->next;
                    found = true;
                    break;
                }
                prev = prev->next;
            }
        }
        if(found) { //空闲时合并
            span->numPages += nextSpan->numPages;
            spanMap.erase(nextAddr);
            delete nextSpan;
        }
    }
    // 将归还span插入空闲列表
    auto& list = freeSpans[span->numPages];
    span->next = list;
    list = span;
}
void* PageCache::systemAlloc(size_t numPages){
    size_t size = numPages * PAGE_SIZE;
    // 使用mman分配内存
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(ptr == MAP_FAILED) return nullptr;
    // 置空内存
    memset(ptr, 0, size);
    return ptr;
}