/// Uses the bufferAllocator as a string stream.
/// Inspired by the Bitsquid foundation library: https://bitbucket.org/bitsquid/foundation/src
#pragma once

#include "memory.h"
#include "bytes.h"

namespace core {
namespace bufferStringStream {

BufferAllocator& printf(BufferAllocator& buffer, const char* format, ...);

BufferAllocator& operator<<(BufferAllocator& buffer, char c);
BufferAllocator& operator<<(BufferAllocator& buffer, const char *s);
BufferAllocator& operator<<(BufferAllocator& buffer, core::Bytes str);
BufferAllocator& operator<<(BufferAllocator& buffer, float f);
BufferAllocator& operator<<(BufferAllocator& buffer, double d);
BufferAllocator& operator<<(BufferAllocator& buffer, int32 i);
BufferAllocator& operator<<(BufferAllocator& buffer, uint32 i);
BufferAllocator& operator<<(BufferAllocator& buffer, uint64 i);
const char* asCString(BufferAllocator& buffer);

} }