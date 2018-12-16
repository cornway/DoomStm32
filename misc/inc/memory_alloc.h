#ifndef __ALLOCATOR
#define __ALLOCATOR

#include <stdint.h>
#include <errno.h>
#include "ArrayListStatic.h"


#ifndef MemoryAlignment
#define MemoryAlignment 4 /*4 bytes*/
#endif



typedef uint32_t memory_size_t;
typedef uint32_t alloc_addr_t; 

__align(MemoryAlignment) class MemoryChunk : public DefaultArrayListBaseNode <MemoryChunk> {
  public:
	  MemoryChunk () : DefaultArrayListBaseNode() {}
      bool equals (MemoryChunk &c)   
      {
          if ((alloc_addr_t)&c >= (alloc_addr_t)this) {
            return true;
          }
          return false;
      }
    private:
	  memory_size_t size;
	friend class MemoryAllocator;
};

#define __m sizeof(MemoryChunk)

class MemoryAllocator {
	public:
	  MemoryAllocator () {}
      MemoryAllocator (uint32_t, uint32_t);
	  void operator () (uint32_t, uint32_t);
	  void *New (memory_size_t) __attribute__((__malloc__));
	  int32_t Delete (void *);	
	  int32_t check ();	
      
	private:
	  MemoryChunk *separate (MemoryChunk *P, memory_size_t Size);
	  int32_t sanitize (ArrayListBase <MemoryChunk> &List);
	  ArrayListBase <MemoryChunk> poolFree, poolInUse;
      uint32_t Success, errors, sanitized;
};


#endif /*__ALLOCATOR*/

