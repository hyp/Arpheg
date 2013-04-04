#pragma once

#include "types.h"

namespace core {

class DynamicLibrary {
public:
	DynamicLibrary(const char* name);
	~DynamicLibrary();
	void* get(const char* name,bool optional = false);

private:
	void* handle;
};

}