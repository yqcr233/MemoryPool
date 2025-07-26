#ifndef SIMPLE_MEMORY_POOL_HPP
#define SIMPLE_MEMORY_POOL_HPP

#include "core.hpp"

#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512
struct Slot{ Slot* next; };
class SimpleMemoryPool{
    private:
        size_t BlockSize;
        int SlotSize;
        Slot* firstBlock;
        Slot* curSlot;
        Slot* lastSlot;
        Slot* freeList;
        mutex mutexForFreeList;
        mutex mutexForBlock;    
    private:
        void init(size_t _SlotSize){
            assert(_SlotSize > 0);
            SlotSize = _SlotSize;
            firstBlock = nullptr;
            curSlot = nullptr;
            freeList = nullptr;
            lastSlot = nullptr;
        }
        size_t padPointer(char* p, size_t align){ // 计算让指针对齐到槽大小的倍数位置需要填充字节数，align是槽大小
            return (align - reinterpret_cast<size_t>(p) % align) % align;
        }
        void allocateNewBlock() {
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

            freeList = nullptr;
        }   
    public:
        SimpleMemoryPool(size_t _SlotSize, size_t _BlockSize = 4096): BlockSize(_BlockSize) { init(_SlotSize); }
        ~SimpleMemoryPool() {
            Slot* cur = firstBlock;
            while(cur) {
                Slot* next = cur->next;
                // 转化为void*, 因为void类型不需要调用析构函数，只释放空间
                operator delete(reinterpret_cast<void*>(cur));
                cur = next;
            }
        }
        void* allocate();
        void deallocate(void*);
};
#endif