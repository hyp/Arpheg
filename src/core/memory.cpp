#include <stdlib.h>
#include <string.h>
#include "assert.h"
#include "branchHint.h"
#include "memory.h"

namespace core {

	//A dummy reallocate implementation.
	void* Allocator::reallocate(void* p,size_t oldSize,size_t newSize,uint32 align){
		auto ptr = allocate(newSize,align);
		memcpy(ptr,p,oldSize);
		deallocate(p);
		return ptr;
	}

	BufferAllocator::BufferAllocator(size_t startingSize,Allocator* backer,uint32 policy) {
		assert(startingSize);
		auto allocator = backer? backer : memory::globalAllocator();
		begin_ = (uint8*)allocator->allocate(startingSize,startingSize >= kAllocatorRegionAlignment? kAllocatorRegionAlignment : kDefaultAlignment);
		end_   = begin_ + startingSize;
		allocate_ = begin_;
		allocator_ = backer;
		policy_ = policy;
	}
	BufferAllocator::BufferAllocator(core::Bytes fixedStorage,uint32 policy,Allocator* growOnOverflowBacker) {
		assert(!fixedStorage.empty());
		begin_ = fixedStorage.begin;
		end_ = fixedStorage.end;
		allocate_ = begin_;
		allocator_ = growOnOverflowBacker;
		policy_ = InFixedStorage|policy;
	}
	BufferAllocator::~BufferAllocator() {
		if(begin_){
			if((policy_ & InFixedStorage) == 0)
				(allocator_? allocator_ : memory::globalAllocator())->deallocate(begin_);
		}
	}
	void* BufferAllocator::allocate(size_t size,uint32 align) {
		if(LIKELY_FALSE(allocate_ + (size+align) > end_)){
			if(!grow(size+align)) return nullptr;
		}
		uintptr_t pi = uintptr_t(allocate_);
		if(align > 1){
			const uint32 mod = pi % align;
			if (mod)
				pi += (align - mod);
		}
		allocate_ = (uint8*)(pi) + size;
		return (void*)pi;
	}
	void  BufferAllocator::reset(size_t newSize) {
		allocate_ = begin_;
		if(newSize){
			auto backer = allocator_? allocator_ : memory::globalAllocator();
			if(!(policy_ & InFixedStorage))
				backer->deallocate(begin_);
			begin_ = (uint8*)backer->allocate(newSize,kAllocatorRegionAlignment);
			end_   = begin_ + newSize;
			allocate_ = begin_;
			policy_ |= (~InFixedStorage);//Boom outta stack!
		}
	}
	void  BufferAllocator::clear() {
		assert(begin_);
		if((policy_ & InFixedStorage) == 0){
			(allocator_? allocator_ : memory::globalAllocator())->deallocate(begin_);
			begin_ = nullptr;
			allocate_ = nullptr;
			end_ = nullptr;
		}
	}
	bool BufferAllocator::grow(size_t requiredAmount) {
		if(policy_ & GrowOnOverflow){
			
			size_t currentSize = (size_t)(end_ - begin_);
			if(!currentSize) currentSize = 1024;
			size_t newSize = (currentSize*2 < currentSize + requiredAmount)? currentSize + requiredAmount : currentSize * 2;
			auto offset = allocate_ - begin_;
			if(policy_ & InFixedStorage){
				auto b = begin_;
				begin_ = (uint8*)(allocator_? allocator_ : memory::globalAllocator())->allocate(newSize,kAllocatorRegionAlignment);
				memcpy(begin_,b,offset);
			}
			else begin_  = (uint8*)(allocator_? allocator_ : memory::globalAllocator())->reallocate(begin_,currentSize,newSize,kAllocatorRegionAlignment);
			end_    = begin_ + newSize;
			allocate_ = begin_ + offset;
			policy_ |= (~InFixedStorage);//Boom outta stack!
			return true;
		}
		else if(policy_ & ResetOnOverflow){
			reset();
			if(requiredAmount >= capacity()){
				assert(false && "Buffer allocator out of space!");
				return false;
			}
			return true;
		}
		else {
			assert(false && "Buffer allocator out of space!");
			return false;
		}
	}
	void  BufferAllocator::deallocate(void* p) {
		//It's a Noop!
	}
	void  BufferAllocator::deallocateFromTop(size_t amount) {
		assert(allocate_ - amount >= begin_);
		allocate_ -= amount;
	}
	uint32 BufferAllocator::toOffset(void* p) {
		assert(p >= begin_ && p < end_);
		return (uint32)((uint8*)p - begin_);
	}
	void*  BufferAllocator::toPointer(uint32 offset){
		//NB: <= instead of < (usage in rendering/opengl/textures.cpp) 
		assert(offset <= uintptr_t(end_ - begin_));
		return begin_ + offset;
	}

	PieceAllocator::PieceAllocator(size_t pieceSize,Allocator* backer){
		allocate_ = end_ = nullptr;
		pieceSize_ = pieceSize;
		allocator_ = backer;
		currentPiece_ = nullptr;
	}
	PieceAllocator::~PieceAllocator(){
		auto alloc = allocator_? allocator_ : memory::globalAllocator();
		for(;currentPiece_ != nullptr;){
			auto next = *( (void**)currentPiece_ );
			alloc->deallocate(currentPiece_);
			currentPiece_ = next;
		}
	}

	void* PieceAllocator::allocate(size_t size,uint32 align){
		if(LIKELY_FALSE(allocate_ + (size+align) > end_)){
			if(!newPiece()) return nullptr;
		}
		uintptr_t pi = uintptr_t(allocate_);
		if(align > 1){
			const uint32 mod = pi % align;
			if (mod)
				pi += (align - mod);
		}
		allocate_ = (uint8*)(pi) + size;
		return (void*)pi;
	}
	bool PieceAllocator::newPiece() {
		auto oldPiece = currentPiece_;
		currentPiece_ = (allocator_? allocator_ : memory::globalAllocator())->allocate(pieceSize_,kAllocatorRegionAlignment);
		*( (void**)currentPiece_ ) = oldPiece;
		allocate_ = ((uint8*)currentPiece_) + sizeof(void*);
		end_ = ((uint8*)currentPiece_) + pieceSize_;
		return true;
	}
	void  PieceAllocator::deallocate(void* p){
		//It's a Noop!
	}


	PoolAllocator::PoolAllocator(size_t blockSize,size_t maxBlocks,Allocator* backer){
		allocator_ = backer;
	}
	PoolAllocator::~PoolAllocator(){

	}
	void* PoolAllocator::allocate(size_t size,uint32 align) {
		return nullptr;
	}
	void  PoolAllocator::deallocate(void* p) {

	}

	class MallocAllocator: public Allocator {

		struct Header {
			uint32 padBegin;
			static const uint32 padValue = 0;

			void fill(void* data){
				padBegin = uint32(this);
				uint32 *p = (uint32 *)(this + 1);
                while (p < data)
                   *p++ = padValue;
			}
			static Header* header(void* data){
                uint32_t *p = (uint32_t *)data;
                while (p[-1] == padValue) --p;
                return ((Header *)p) - 1;
			}
		};

		void* allocate  (size_t size,uint32 align) {
			assert(align >= 4);
			size = size + sizeof(Header) + align;
			Header* header = (Header*)malloc(size);
			auto p = memory::align_forward(header + 1,align);
			header->fill(p);
			return p;//malloc(size);
		}
		void  deallocate(void* p) {
			//free(p);
			auto header = Header::header(p);
			free(header);
		}
	};
	MallocAllocator mallocAllocator;
	Allocator*  memory::globalAllocator() {
		return &mallocAllocator;
	}
}