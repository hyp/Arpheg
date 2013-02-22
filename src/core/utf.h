#pragma once

#include "types.h"

namespace core {
namespace utf8  {
	//NB: the leading character is assumed to be > 127
	uint32 sequenceLength(uint8  leadingCharacter);
	uint32 decode(uint32 sequenceLength,const uint8* sequence);
	//Returns the encoded sequence length
	uint32 encode(uint32 codepoint,uint8* dest);
} }