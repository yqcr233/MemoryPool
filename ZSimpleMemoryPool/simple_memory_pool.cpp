#include "simple_memory_pool.hpp"
void MemoryPool::init(size_t _SlotSize){
    assert(_SlotSize > 0);
    SlotSize = _SlotSize;
    firstBlock = nullptr;
    curSlot = nullptr;
    freeList = nullptr;
    lastSlot = nullptr;
}
void* MemoryPool::allocate(){
    // 优先使用freeList空闲链表中的内存槽
    if(freeList != nullptr) {
        lock_guard<std::mutex> lock(mutexForFreeList);
        if(freeList != nullptr) {
            Slot* temp = freeList;
            freeList = freeList->next;
            return temp;
        }
    } 

    // freeList中无空闲内存槽，则使用内存块中未使用的内存槽
    Slot* temp = nullptr;
    {
        lock_guard<std::mutex> lock(mutexForBlock);
        if(curSlot >= lastSlot) { // 剩余内存不足一个内存槽则重新申请一个内存池
            allocateNewBlock();
        }
        temp = curSlot;
        curSlot += SlotSize / sizeof(Slot); // curSlot是slot类型指针，步长单位为sizeof(Slot)
    }
    return  temp;
}
void MemoryPool::deallocate(void* ptr){
    if(ptr) {
        // 回收内存，并将内存头插入freeList中
        lock_guard<std::mutex> lock(mutexForFreeList);
        reinterpret_cast<Slot*>(ptr)->next = freeList;
        freeList = reinterpret_cast<Slot*>(ptr);
    }
}
void HashBucket::initMemoryPool(){
    for(int i = 0; i < MEMORY_POOL_NUM; i++) {
        getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
    }
}
MemoryPool& HashBucket::getMemoryPool(int index){
    // 单例懒汉模式，使用局部静态变量创建内存池实例 
    static MemoryPool memoryPool[MEMORY_POOL_NUM];
    return memoryPool[index];
}
void* HashBucket::useMemory(size_t size){
    if(size < 0) {
        return nullptr;
    }
    if(size > MAX_SLOT_SIZE) {
        return operator new(size); // 直接向堆中申请大内存块
    }
    return getMemoryPool((size + 7) / SLOT_BASE_SIZE - 1).allocate();
}
void HashBucket::freeMemory(void* ptr, size_t size){
    if(!ptr) {
        return;
    }
    if(size > MAX_SLOT_SIZE) {
        operator delete(ptr);
        return;
    }   
    getMemoryPool((size + 7) / SLOT_BASE_SIZE - 1).deallocate(ptr);
}
