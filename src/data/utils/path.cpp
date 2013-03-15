#include <string.h>
#include "path.h"

namespace data {
namespace utils {
namespace path {

void join(core::BufferAllocator& buffer,core::Bytes a,core::Bytes b) {
	using namespace core::bufferStringStream;

	//TODO better version (win32 '\\' support)
	buffer<<a;
	if(a.length() && *(a.end-1) != '/') buffer<<"/";
	buffer<<b;
}
void dirname(core::BufferAllocator& buffer,core::Bytes path){
	using namespace core::bufferStringStream;
	if(path.empty()) return;
	auto p = path.begin+path.length()-1;
	while(p>path.begin && *p != '/' && *p != '\\') p--;
	buffer<<core::Bytes((void*)path.begin,(void*)(p+1));
}
const char* extension(const char* path) {
	auto len = strlen(path);
	for(size_t i = len;i!=0;){
		if(path[i-1] == '.') return path + i;
		i--;
	}
	return path+len;
}

} } }