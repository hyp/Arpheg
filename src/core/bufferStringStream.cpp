#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "bufferStringStream.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace core {
namespace bufferStringStream {

BufferAllocator& printf(BufferAllocator& buffer, const char* format, ...) {
    va_list args;
                        
    va_start(args, format);
    int n = vsnprintf((char*)nullptr, 0, format, args);
    va_end(args);

	auto ptr = buffer.allocate(n+1,0);
                        
    va_start(args, format);
    vsnprintf((char*)ptr, n+1, format, args);
    va_end(args);

	buffer.deallocateFromTop(1);//Say no to \0!
	return buffer;
}

template <typename T>
static inline void printf_small(BufferAllocator& buffer, const char *fmt, T t) {
    char s[32];
    auto len = snprintf(s, 32, fmt, t);
	memcpy(buffer.allocate(len,0),s,len);
}

BufferAllocator& operator<<(BufferAllocator& buffer, char c){
	*( (char*)buffer.allocate(sizeof(char),0) ) = c;
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, const char *s){
	auto len = strlen(s);
	if(len) memcpy(buffer.allocate(len,0),s,len);
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, core::Bytes str){ 
	auto len = str.length();
	if(len) memcpy(buffer.allocate(len,0),str.begin,len);
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, float f){
	printf_small(buffer, "%g", double(f));
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, double d){
	printf_small(buffer, "%g", d);
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, int32 i){
	printf_small(buffer, "%i", uint32(i));
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, uint32 i){
	printf_small(buffer, "%u", i);
	return buffer;
}
BufferAllocator& operator<<(BufferAllocator& buffer, uint64 i){
	printf_small(buffer, "%01llx", i);
	return buffer;
}
const char* asCString(BufferAllocator& buffer){
	// Ensure there is a \0 at the end of the buffer.
	*( (char*)buffer.allocate(sizeof(char),0) ) = '\0';
	buffer.deallocateFromTop(1);
	return (const char*) buffer.bufferBase();
}

} }