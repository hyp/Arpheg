#pragma once

#include <stdio.h>
#include "types.h"
#include "memoryTypes.h"

namespace io {

	// A wrapper for stdio FILE* which closes the file on scope exit
	struct File {
		enum Mode {
			Read = 1 ,Write = 2,Append = 4,Insert = 8,
			Text = 0x10,
		};
		FILE* fptr;

		File(const char* path,uint32 mode = Read);
		~File();
		inline operator FILE* ();

	private:
		inline File(const File& other) {}
		inline void operator = (const File& other) { }
	};
	inline File::operator FILE* () { return fptr; }

	// A structure which loads the file into memory when created and releases the memory when destroyed
	struct Data {
		uint8* begin;
		size_t size;
		core::Allocator* allocator;

		Data(const char* path,core::Allocator* allocator = nullptr,size_t alignment = 16);
		~Data();
	};
}
