#include "thread_cache.hpp"

void* ThreadCache::allocate(size_t size){
    if(size == 0) { // 至少分配一个对齐大小
        size = ALIGNMENT;
    }
    if(size > MAX_BYTES) {
        return malloc(size);
    }
    size_t index = SizeClass::getIndex(size);
    if(void* ptr = freeList[index]) {
        freeListSize[index] -= 1;
        freeList[index] = *reinterpret_cast<void**>(ptr); // C++禁止对void*进行解引用
        return ptr;
    }
    return fetchFromCentralCache(index);
}
void ThreadCache::deallocate(void* ptr, size_t size){
    if(size > MAX_BYTES){
        free(ptr);
        return;
    }

    size_t index = SizeClass::getIndex(size);
    *reinterpret_cast<void**>(ptr) = freeList[index];
    freeList[index] = ptr;
    freeListSize[index] += 1;
    if(shouldReturnToCentralCache(index)){
        returnToCentralCache(freeList[index], size);
    }
}
void* ThreadCache::fetchFromCentralCache(size_t index){
    void* start = CentralCache::getInstance().fetchRange(index);
    if(!start) {
        return nullptr;
    }
    // 取第一个返回其余放入freeList中
    void* result = start;
    freeList[index] = *reinterpret_cast<void**>(start);
    size_t batchSize = 0;
    void* current = *reinterpret_cast<void**>(start);
    // 计算从中心缓存中获取的内存块数量
    while(current != nullptr) {
        batchSize += 1;
        current = *reinterpret_cast<void**>(current);
    }
    freeListSize[index] += batchSize;
    return result;
}
bool  ThreadCache::shouldReturnToCentralCache(size_t index){
    return freeListSize[index] > THRESHOULD;
}
void ThreadCache::returnToCentralCache(void* start, size_t size){
    // 计算需要规划的块索引和实际内存块大小
    size_t index = SizeClass::getIndex(size);
    size_t aligedSize = SizeClass::roundUP(size);
    // 计算所需归还数量
    size_t batchNum = freeListSize[index];
    if(batchNum <= 1) return ;
    size_t keepNum = max(batchNum / 4, size_t(1));
    size_t returnNum = batchNum - keepNum;

    char* spiltNode = static_cast<char*>(start);
    // 检查自由链表是否符合预期
    for(int i = 0; i < keepNum - 1; i++) { 
        spiltNode = reinterpret_cast<char*>(*reinterpret_cast<void**>(spiltNode));
        if(spiltNode == nullptr) {
            freeListSize[index] = i + 1; // 更新自由链表实际长度
            break;
        }
    }
    if(spiltNode != nullptr) {
        void* nextNode = *reinterpret_cast<void**>(spiltNode);
        *reinterpret_cast<void**>(spiltNode) = nullptr; // 断开链接
        freeList[index] = start;
        freeListSize[index] = keepNum;
        if(returnNum > 0 && nextNode != nullptr) {
            CentralCache::getInstance().returnRange(nextNode, returnNum * aligedSize, index);
        }
    }
}
