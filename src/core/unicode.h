#pragma once

#include "types.h"

namespace core {
namespace unicode {

const uint32 max = 0x10FFFF;
const uint32 surrogatePairRangeBegin = 0xD800;
const uint32 surrogatePairRangeEnd   = 0xDFFF;

inline bool isSurrogate(uint32 codepoint){
	return surrogatePairRangeBegin <= codepoint && codepoint <= surrogatePairRangeEnd;
}
inline bool isValid(uint32 codepoint){
	return codepoint <= max && !(surrogatePairRangeBegin <= codepoint && codepoint <= surrogatePairRangeEnd);
}

} }