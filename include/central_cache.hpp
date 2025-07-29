#ifndef central_cache_HPP
#define central_cache_HPP
#include "core.hpp"
class CentralCache{
    public:
        static CentralCache& getInstance(){
            static CentralCache instance;
            return instance;
        }
        void* fetchRange(size_t range);
        void returnRange(void* start, size_t size, size_t index);
    private:

};
#endif