#ifndef SIMPLE_MEMORY_POOL_HPP
#define SIMPLE_MEMORY_POOL_HPP

#include "core.hpp"

#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512
struct Slot{ Slot* next; };
class MemoryPool{
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
        size_t padPointer(char* p, size_t align){ // 计算让指针对齐到槽大小的倍数位置需要填充字节数，align是槽大小
            return (align - reinterpret_cast<size_t>(p) % align) % align; // 将p转换成size_t(整数类型)便于进行取模运算
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
        MemoryPool(size_t _BlockSize = 4096): BlockSize(_BlockSize) {}
        ~MemoryPool() {
            Slot* cur = firstBlock;
            while(cur) {
                Slot* next = cur->next;
                // 转化为void*, 因为void类型不需要调用析构函数，只释放空间
                operator delete(reinterpret_cast<void*>(cur));
                cur = next;
            }
        }
        void init(size_t _SlotSize);
        void* allocate();
        void deallocate(void*);

        friend class FastMemoryPool;
};

class HashBucket{
    public:
        static void initMemoryPool();
        static MemoryPool& getMemoryPool(int index);
        static void* useMemory(size_t size);
        static void freeMemory(void* ptr, size_t size);

        template<typename T, typename... Args>
        friend T* newElement(Args&&... args);
        template<typename T> 
        friend void deleteElement(T* p);
};

template<typename T, typename... Args>
T* newElement(Args&&... args){
    T* p = nullptr;
    if((p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T)))) != nullptr) { // 从内存池中获取适合内存槽
        new(p) T(forward<Args>(args)...); // 在对应内存上调用构造函数
    }
    return p;
}
template<typename T> 
void deleteElement(T* p){
    if(p) {
        // 调用析构并回收内存
        p->~T();
        // 这里将p从T*转为void*只要是将内存池与具体类型解耦，避免内存池代码需要模板实例化，使用void*则不需要提前知道p的类型
        // freeMemory只需要知道待释放内存首地址和待释放内存大小，与内存中所存储数据类型无关 
        HashBucket::freeMemory(reinterpret_cast<void*>(p), sizeof(T)); 
    }
}
#endif