#pragma once
#include "../core/assert.h"

namespace scene {
namespace rendering {

template<typename T>
struct EntityPool {
	size_t advance;
	T* firstFree;
	core::BufferAllocator buffer;

	EntityPool(uint32 count,core::Allocator* backer): buffer(sizeof(T)*count,backer) {
		advance = 0;firstFree = nullptr;
	}
	inline T** nextDest(T* p){
		return (T**)p;
	}
	T* allocate(){
		if(advance < buffer.capacity()){
			advance += sizeof(T);
			return (T*)buffer.allocate(sizeof(T),0);
		} else {
			if(!firstFree){
				assert(false && "Pool is out of size!");
				return nullptr;
			}
			auto result = firstFree;
			firstFree = *nextDest(firstFree);
			return result;
		}
	}
	void free(T* entity){
		*nextDest(entity) = firstFree;
		firstFree = entity;
	}
	uint32 toId(T* ent){
		return buffer.toOffset(ent)/sizeof(T);
	}
	T* toPtr(uint32 id){
		return (T*)buffer.toPointer(id*sizeof(T));
	}
};

} }