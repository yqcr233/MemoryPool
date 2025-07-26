#include "simple_memory_pool.hpp"

void* SimpleMemoryPool::allocate(){
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
void SimpleMemoryPool::deallocate(void* ptr){
    if(ptr) {
        // 回收内存，并将内存头插入freeList中
        lock_guard<std::mutex> lock(mutexForFreeList);
        reinterpret_cast<Slot*>(ptr)->next = freeList;
        freeList = reinterpret_cast<Slot*>(ptr);
    }
}