#include "../core/assert.h"
#include "memory.h"

#define USE_DL_PREFIX
#define MALLOC_ALIGNMENT 16
#define MSPACES 1

#include "../dependencies/dlmalloc/dlmalloc.c"

namespace core {
	HeapAllocator::HeapAllocator(size_t size){
		heap_ = create_mspace(size,0);
		mspace_track_large_chunks(heap_, 1);
	}
	HeapAllocator::~HeapAllocator(){
		destroy_mspace(heap_);
	}
	void* HeapAllocator::allocate  (size_t size,uint32 align){
		assert(align <= MALLOC_ALIGNMENT);
		auto p = mspace_malloc(heap_, size);
		assert(uintptr_t(p) % align == 0);
		return p;
	}
	void  HeapAllocator::deallocate(void* p){
		mspace_free(heap_,p);
	}
}

#undef MALLOC_ALIGNMENT
#undef MSPACES
#undef USE_DL_PREFIX
