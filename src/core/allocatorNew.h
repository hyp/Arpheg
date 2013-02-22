#pragma once

#include "types.h"
#include <new>

#define ALLOCATOR_NEW(allocator,Type) new((allocator)->allocate(sizeof(Type),alignof(Type))) Type