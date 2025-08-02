#include "fast_memory_pool.hpp"
/**
 * 使用CAS机制进行freeList的出队和入队操作
 */
void FastMemoryPool::init(size_t _SlotSize){
    assert(_SlotSize > 0);
    SlotSize = _SlotSize;
    firstBlock = nullptr;
    curSlot = nullptr;
    lastSlot = nullptr;
    freeList.store(nullptr);
}
void* FastMemoryPool::allocate(){
    // 优先使用freeList空闲链表中的空闲槽
    Slot* slot = popFreeList();
    if(slot != nullptr) {
        return slot;
    }
    // freeList无空闲槽则查找未使用内存槽
    lock_guard<std::mutex> lock(mutexForBlock);
    if(curSlot >= lastSlot) {
        allocateNewBlock();
    }
    Slot* result = curSlot;
    curSlot = reinterpret_cast<Slot*>(reinterpret_cast<char*>(curSlot) + SlotSize);
    return result;    
}
void FastMemoryPool::deallocate(void* ptr){
    if(!ptr) return;
    Slot* slot = static_cast<Slot*>(ptr);
    pushFreeList(slot);
}
size_t FastMemoryPool::padPointer(char* p, size_t align){
    return (align - reinterpret_cast<size_t>(p) % align) % align;
}
void FastMemoryPool::allocateNewBlock(){
    std::cout << "申请一块内存块，SlotSize: " << SlotSize << std::endl;
    void* newBlock = operator new(BlockSize); // void*通用指针类型，这里用来表未类型化的内存块
    // 头插法插入内存块
    reinterpret_cast<Slot*>(newBlock)->next = firstBlock;
    firstBlock = reinterpret_cast<Slot*>(newBlock);
    // 填充队头next指针并初始化curslot(还未使用slot)
    char* body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);
    size_t paddingSize = padPointer(body, SlotSize); // 将当前内存块实际使用区域对齐到SlotSize
    curSlot = reinterpret_cast<Slot*>(body + paddingSize);
    // 设置lastSlot
    lastSlot = reinterpret_cast<Slot*>(reinterpret_cast<size_t>(newBlock) + BlockSize - SlotSize + 1);

    freeList.store(nullptr);
}
bool FastMemoryPool::pushFreeList(Slot* slot){
    while(true) { // 使用自旋锁轮询freeList状态
        Slot* old_head = freeList.load();
        slot->next.store(old_head);
        if(freeList.compare_exchange_weak(old_head, slot)){
            return true;    
        }
    }
}
Slot* FastMemoryPool::popFreeList(){
    while(true) {
        Slot* old_head = freeList.load();
        if(old_head == nullptr) return nullptr;
        Slot* new_head = old_head->next.load();
        if(freeList.compare_exchange_weak(old_head, new_head)) {
            return old_head;
        }
    }
}