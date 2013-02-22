#pragma once

#include "branchHint.h"
#include "dataProvider.h"

namespace io {

class CharSource {
public:
	CharSource(const char* string);
	CharSource(core::Bytes data);
	CharSource(DataProvider* provider);

	inline uint8 consume();
	inline uint8 peek();
	inline uint8 peek(int amount);
	uint32 peekUtf8(uint32& length);
	uint32 peekUtf8(uint32& length,int offset);
	inline void skip(int amount);
	void read(int amount,uint8* buffer);
	inline uint32 position();
protected:
	void moveToBuffer();
	void refill(int lookahead = 0);
	void moveBuffer();

	uint8* src,*end;
	core::Bytes data;
	uint32 endOffset;
	DataProvider* provider;
	enum { kBufferSize = 512 };
	uint8 buffer[kBufferSize];//Buffer for peeking.
};

inline uint8 CharSource::consume(){
	if(LIKELY_FALSE(src >= end)) refill();
	return *src++;
}
inline uint8 CharSource::peek(){
	if(LIKELY_FALSE( src >= end)) refill();
	return *(src);
}
inline uint8 CharSource::peek(int amount){
	//assert(amount < kBufferSize);
	if(LIKELY_FALSE( (src + amount) >= end)) refill(amount);
	return *(src+amount);
}
inline void CharSource::skip(int amount) {
	read(amount,nullptr);
}
inline uint32 CharSource::position(){
	return endOffset - uint32(end - src);
}

}