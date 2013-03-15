// Provides some utility functions for filesystem path manipulation

#pragma once

#include "../../core/bytes.h"
#include "../../core/memory.h"
#include "../../core/bufferStringStream.h"

namespace data {
namespace utils {
namespace path {

//TODO unittest this shit
void join(core::BufferAllocator& buffer,core::Bytes a,core::Bytes b);
void dirname(core::BufferAllocator& buffer,core::Bytes path);
const char* extension(const char* filename); 

} } }