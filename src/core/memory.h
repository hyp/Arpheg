#pragma once

#include "types.h"
#include "bytes.h"

namespace core {

	class Allocator {
	public:
		enum {
			kDefaultAlignment = 4,
			kAllocatorRegionAlignment = 16
		};

		Allocator() { }
		virtual ~Allocator() {} ;

		virtual void* allocate  (size_t size,uint32 align = kDefaultAlignment) = 0;
		virtual void  deallocate(void* p) = 0;
		virtual void* reallocate(void* p,size_t oldSize,size_t newSize,uint32 align = kDefaultAlignment);

	private:
		Allocator(const Allocator& other);
		Allocator& operator= (const Allocator& other);
	};

	//Allocates memory linearly from a single buffer.
	//The buffer can be set to grow or reset on overflow.
	//Very useful as a memory provider for various data structures.
	class BufferAllocator: public Allocator {
	public:
		enum {
			GrowOnOverflow = 0x1,
			ResetOnOverflow = 0x2,
			Lazy = 0x4,
		};
		//Dynamic memory allocation - allocates startingSize bytes from the backer or a default allocator.
		BufferAllocator(size_t startingSize,Allocator* backer = nullptr,uint32 policy = 0);
		BufferAllocator(core::Bytes fixedStorage,uint32 policy = 0,Allocator* growOnOverflowBacker = nullptr);
		~BufferAllocator();

		void* allocate  (size_t size,uint32 align = kDefaultAlignment);
		void  deallocate(void* p);
		void  reset(size_t newSize = 0);
		void  clear();
		void  deallocateFromTop(size_t amount);

		inline uint8* bufferBase() const;
		inline uint8* bufferTop() const;
		uint32 toOffset(void* p);
		void*  toPointer(uint32 offset);

		inline size_t size() const;
		inline size_t capacity() const;
		inline bool canAllocate(size_t size,uint32 align = kDefaultAlignment) const;
	protected:
		enum {
			InFixedStorage = 0x8,
		};
	private:
		bool grow(size_t requiredAmount);
		uint8* begin_,*end_;
		uint8* allocate_;
		Allocator* allocator_;
		uint32 policy_;
	};
	inline uint8* BufferAllocator::bufferBase() const { return begin_; }
	inline uint8* BufferAllocator::bufferTop() const  { return allocate_; }
	inline size_t BufferAllocator::size()  const { return size_t(allocate_ - begin_); }
	inline size_t BufferAllocator::capacity() const { return size_t(end_ - begin_); }
	inline bool BufferAllocator::canAllocate(size_t size,uint32 align) const {
		return (allocate_ + (size+align)) <= end_;
	}

	//Piece allocator is similar to buffer allocator, but it has several chained buffers
	class PieceAllocator: public Allocator {
	public:
		enum { DefaultPieceSize = 1024*4 };//4KiB

		PieceAllocator(size_t pieceSize = DefaultPieceSize,Allocator* backer = nullptr);
		~PieceAllocator();

		void* allocate(size_t size,uint32 align = kDefaultAlignment);
		void  deallocate(void* p);
	private:
		bool newPiece();
		uint8* allocate_,*end_;
		void*  currentPiece_;
		size_t pieceSize_;
		Allocator* allocator_;
	};

	class PoolAllocator: public Allocator {
		PoolAllocator(size_t blockSize,size_t maxBlocks = 0,Allocator* backer = nullptr);
		~PoolAllocator();

		void* allocate(size_t size =0,uint32 align = kDefaultAlignment);
		void  deallocate(void* p);
	private:
		Allocator* allocator_;
	};

	namespace memory {
		inline void *align_forward(void *p, uint32 align) {
			uintptr_t pi = uintptr_t(p);
			const uint32 mod = pi % align;
			if (mod)
				pi += (align - mod);
			return (void *)pi;
		}
		Allocator*   globalAllocator();
	}

}