#include "assert.h"
#include "memory.h"
#include "io.h"

namespace io {
	File::File(const char* path,uint32 mode) {
		char fileMode[5] = { '\0' };
		int offset = 0;
		if(mode & Append) fileMode[offset++]  = 'a';
		if(mode & Read)   fileMode[offset++]  = 'r';
		if(mode & Write)  fileMode[offset++]  = 'w';
		if(mode & Insert) fileMode[offset++]  = '+';
		if(!(mode & Text)) fileMode[offset++] = 'b';
		fptr = fopen(path,fileMode);
		if(!fptr)
			assert(false && "Failed to open a file!");
	}
	File::~File() {
		if(fptr) fclose(fptr);
		fptr = nullptr;
	}

	Data::Data(const char* path,core::Allocator* allocator,size_t alignment){
		if(!allocator) allocator = core::memory::globalAllocator();
		this->allocator = allocator;
		File file(path,File::Read);
		//TODO replace?
		fseek(file,0,SEEK_END);
		size = ftell(file);
		fseek(file,0,SEEK_SET);
		begin = (uint8*)allocator->allocate(size,alignment);
		fread(begin,1,size,file);
	}
	Data::~Data(){
		assert(begin);
		allocator->deallocate(begin);
	}
}