#include "platform.h"
#include "dynamicLibrary.h"
#include "assert.h"
#include "bufferStringStream.h"

#ifdef ARPHEG_PLATFORM_POSIX
#include <dlfcn.h>
#endif

namespace core {

#ifdef ARPHEG_PLATFORM_WIN32

DynamicLibrary::DynamicLibrary(const char* name){
	//NB: extension is appended automatically (MSDN)
	handle = (void*)LoadLibraryA(name);
	if(!handle){
		auto err = GetLastError();
		assertRelease(false && "Failed to load library");
	}
}
DynamicLibrary::~DynamicLibrary(){
	if(handle) FreeLibrary((HMODULE)handle);
}
void*  DynamicLibrary::get(const char* name,bool optional){
	assert(handle);
	auto p = GetProcAddress((HMODULE)handle,name);
	if(!p && !optional) assertRelease(false && "Failed to get function adress!");
	return p;
}

#elif defined(ARPHEG_PLATFORM_POSIX) 

DynamicLibrary::DynamicLibrary(const char* name){
	using namespace bufferStringStream;
	Formatter fmt;
	printf(fmt.allocator,"%s.so",name);

	handle = dlopen(asCString(fmt.allocator),RTLD_NOW);
	if(!handle){
		assertRelease(false && "Failed to load library");
	}
}
DynamicLibrary::~DynamicLibrary(){
	if(handle) dlclose(handle);
}
void*  DynamicLibrary::get(const char* name,bool optional){
	assert(handle);
	auto p = dlsym(handle,name);
	if(!p && !optional) assertRelease(false && "Failed to get function adress!");
	return p;
}

#endif

}