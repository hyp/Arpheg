#include "assert.h"
#include "unicode.h"
#include "utf.h"

namespace core {
namespace utf8 {

uint32 sequenceLength(uint8 leadingCharacter){
	uint32 n = 1;
	for (;; n++) {
		if (((leadingCharacter << n) & 0x80) == 0)
			break;
	}
	return n;
}
uint32 decode(uint32 sequenceLength,const uint8* sequence){
	auto leadingCharacter = sequence[0];
	if(sequenceLength == 1) return leadingCharacter;
	if(sequenceLength > 4) return 0;
	// Pick off (7 - n) significant bits of B from first byte of octet
	uint32_t result = (leadingCharacter & ((1 << (7 - sequenceLength)) - 1));
		
	/* The following combinations are overlong, and illegal:
	*  1100000x (10xxxxxx)
	*  11100000 100xxxxx (10xxxxxx)
	*  11110000 1000xxxx (10xxxxxx 10xxxxxx)
	*  11111000 10000xxx (10xxxxxx 10xxxxxx 10xxxxxx)
	*  11111100 100000xx (10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx)
	*/
	if ((leadingCharacter & 0xFE) == 0xC0 ||
		(leadingCharacter == 0xE0 && (sequence[1] & 0xE0) == 0x80) ||
		(leadingCharacter == 0xF0 && (sequence[1] & 0xF0) == 0x80) ||
		(leadingCharacter == 0xF8 && (sequence[1] & 0xF8) == 0x80) ||
		(leadingCharacter == 0xFC && (sequence[1] & 0xFC) == 0x80))
		return 0; // overlong combination
	
	auto end = sequence + sequenceLength;
	for (sequence++; sequence < end; sequence++) {
		if ((*sequence & 0xC0) != 0x80)
			return 0; // trailing bytes are 10xxxxxx
		result = (result << 6) | (((uint32) (*sequence)) & 0x3F);
	}
	return result;
}
uint32 encode(uint32 c,uint8* buf){
	if (c <= 0x7F) {
		buf[0] = (uint8) c;
		return 1;
	}
	else if (c <= 0x7FF) {
		buf[0] = (uint8) (0xC0 | (c >> 6));
		buf[1] = (uint8) (0x80 | (c & 0x3F));
		return 2;
	}
	else if (c <= 0xFFFF) {
		if(!unicode::isSurrogate(c)){
			buf[0] = (uint8) (0xE0 | (c >> 12));
			buf[1] = (uint8) (0x80 | ((c >> 6) & 0x3F));
			buf[2] = (uint8) (0x80 | (c & 0x3F));
			return 3;
		}
	}
	else if (c <= unicode::max) {
		buf[0] = (uint8) (0xF0 | (c >> 18));
		buf[1] = (uint8) (0x80 | ((c >> 12) & 0x3F));
		buf[2] = (uint8) (0x80 | ((c >> 6) & 0x3F));
		buf[3] = (uint8) (0x80 | (c & 0x3F));
		return 4;
	}
	assert(false && "Invalid unicode codepoint");
	return 0;
}

} }