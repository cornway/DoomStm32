

#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>
#include <iterable.h>

#define MEM_WORD uint32_t
#define MEM_MAGIC 0xaa10
#define MEM_CHUNK_SIZE 16
class MemoryPool;

class MemoryChunk : public Link<MemoryChunk> {
    private :
        MEM_WORD size;

        friend class MemoryPool;
    public :
        MemoryChunk() : delete
        ~MemoryChunk() : delete

};

class MemoryPool {
    private :
        vector::Vector freePool, allocaPool;
    public :
        MemoryPool (uint32_t offset, uint32_t size);
        ~MemoryPool();
};


#endif /*_MEMORY_H*/

