/// Uses the bufferAllocator as an array.
#pragma once

#include "memory.h"

namespace core {
namespace bufferArray {

template<typename T>
inline size_t length(const BufferAllocator& buffer){ return buffer.size()/sizeof(T); }
template<typename T>
inline T* begin(BufferAllocator& buffer){ return (T*)buffer.bufferBase(); }
template<typename T>
inline T* end(BufferAllocator& buffer) { return (T*)buffer.bufferTop(); }

template<typename T>
inline T* allocate(BufferAllocator& buffer) { return (T*)buffer.allocate(sizeof(T),alignof(T)); }
template<typename T>
inline void add(BufferAllocator& buffer,const T& value) { 
	*( (T*)buffer.allocate(sizeof(T),alignof(T)) ) = value;
}
template<typename T>
inline void removeLast(BufferAllocator& buffer) { buffer.deallocateFromTop(sizeof(T)); }
template<typename T>
void add(BufferAllocator& buffer,const T* items,size_t count){
	auto ptr = (T*)buffer.allocate(sizeof(T)*count,alignof(T));
	for(auto end = items+count;items<end;++items,++ptr) *ptr = *items;
}

template<typename T>
inline T& front(BufferAllocator& buffer){ return *(T*)buffer.bufferBase(); }
template<typename T>
inline T& back(BufferAllocator& buffer) { return *( ((T*)buffer.bufferTop()) - 1 ); }
template<typename T>
inline T& nth(BufferAllocator& buffer,int i) { return *( ((T*)buffer.bufferBase()) + i); }

//Some common algorithms
template<typename T>
int indexOf(BufferAllocator& buffer,const T& value) {
	for(T* b = begin<T>(buffer),* e = end<T>(buffer);b < e;++b){
		if(*b == value) return int(b - begin<T>(buffer));
	}
	return -1;
}
template<typename T>
int lastIndexOf(BufferAllocator& buffer,const T& value) {
	for(T* b = end<T>(buffer) - 1,* e = begin<T>(buffer);b >= e;--b){
		if(*b == value) return int(b - begin<T>(buffer));
	}
	return -1;
}
template<typename T>
void replace(BufferAllocator& buffer,const T& value,const T& newValue) {
	for(T* b = begin<T>(buffer),* e = end<T>(buffer);b < e;++b){
		if(*b == value) *b = newValue;
	}
}
template<typename T>
int count(BufferAllocator& buffer,const T& value) {
	int i = 0;
	for(auto b = begin<T>(buffer),e = end<T>(buffer);b < e;++b){
		if(*b == value) i++;
	}
	return i;
}

} }