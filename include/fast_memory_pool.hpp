#ifndef FAST_MEMORY_POOL_HPP
#define FAST_MEMORY_POOL_HPP
#include "core.hpp"
#include "simple_memory_pool.hpp"
#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512
/**
 * 在普通内存池上，使用原子变量代替锁，避免线程阻塞和上下文切换的开销
 */
struct Slot { atomic<Slot*> next; }; // 使用原子指针管理内存槽
class FastMemoryPool {
    private:
        size_t BlockSize;
        int SlotSize;
        Slot* firstBlock;
        Slot* curSlot;
        Slot* lastSlot;
        atomic<Slot*> freeList; // 使用原子指针管理空闲链表队列
        mutex mutexForBlock; // 内存块申请操作使用比较少，直接使用互斥锁管理逻辑更简单
    private:
        size_t padPointer(char* p, size_t align);
        void allocateNewBlock();
        // CAS操作进行freeList的入队和出队操作
        bool pushFreeList(Slot* slot);
        Slot* popFreeList();
    public: 
        FastMemoryPool(size_t _BlockSize = 4096): BlockSize(_BlockSize) {}
        ~FastMemoryPool(){
            Slot* cur = firstBlock;
            while(cur) {
                Slot* next = cur->next;
                // 转化为void*, 因为void类型不需要调用析构函数，只释放空间
                operator delete(reinterpret_cast<void*>(cur));
                cur = next;
            }
        }
        void init(size_t);
        void* allocate();
        void deallocate(void*);
};
#endif