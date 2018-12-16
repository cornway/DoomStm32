

#include "memory.h"
#include "vm.h"


void MemoryChunk::setup (MEM_WORD size, MEM_WORD magic)
{
    this->size = size;
    this->maic = magic;
}


MemoryPool::MemoryPool (uint32_t offset, uint32_t size)
{
    MemoryChunk *c = (MemoryChunk *)offset;
    c->size = size - sizeof(MemoryChunk);
}


