/// Uses the bufferAllocator as a stack.

#include "memory.h"

namespace core {
namespace bufferStack {

template<typename T>
inline bool isEmpty(const BufferAllocator& buffer){ return buffer.size()==0; }
template<typename T>
inline void push(BufferAllocator& buffer,const T& value) { 
	*( (T*)buffer.allocate(sizeof(T),alignof(T)) ) = value;
}
template<typename T>
inline T pop(BufferAllocator& buffer){
	buffer.deallocateFromTop(sizeof(T));
	//NB: when deallocating from top, the memory is still owned by the buffer, therefore this last object is still valid
	return *(T*)buffer.bufferTop();
}
template<typename T>
inline const T& peek(BufferAllocator& buffer){ return *( ((T*)buffer.bufferTop()) - 1 ); }

} }