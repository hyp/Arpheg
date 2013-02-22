#pragma once

#include <stdio.h>
#include "types.h"
#include "memoryTypes.h"

namespace io {

	struct File {
		enum Mode {
			Read = 1 ,Write = 2,Append = 4,Insert = 8,
			Text = 16
		};
		FILE* fptr;

		File(const char* path,uint32 mode = Read);
		~File();
		inline operator FILE* ();

	private:
		File(const File& other) {}
		File operator = (const File& other) { return *this; }
	};
	inline File::operator FILE* () { return fptr; }

	//Represents a file fully loaded into memory
	struct Data {
		uint8* begin;
		size_t size;
		core::Allocator* allocator;

		Data(const char* path,core::Allocator* allocator = nullptr);
		~Data();
	};
}
