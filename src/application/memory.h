#pragma once

#include "../core/memory.h"

namespace core {
	class HeapAllocator: public Allocator {
	public:
		HeapAllocator(size_t size = 0);
		~HeapAllocator();

		void* allocate  (size_t size,uint32 align = kDefaultAlignment);
		void  deallocate(void* p);
	private:
		void* heap_;
	};
}