#include <string.h>
#include "assert.h"
#include "charSource.h"
#include "utf.h"

namespace io {

CharSource::CharSource(const char* string) : data(nullptr,nullptr) {
	src = (uint8*)string;
	end = src + strlen(string);
	provider = nullptr;
	endOffset = uint32(end-src);
}
CharSource::CharSource(core::Bytes d) : data(nullptr,nullptr) {
	src = d.begin;
	end = d.end;
	provider = nullptr;
	endOffset = uint32(end-src);
}
CharSource::CharSource(DataProvider* provider): data(nullptr,nullptr) {
	src = end = buffer;
	this->provider = provider;
	endOffset = 0;
}
uint32 CharSource::peekUtf8(uint32& length){
	uint8  utf8Sequence[4];
	utf8Sequence[0] = peek();

	if(utf8Sequence[0] < 127){
		length = 1;
		return utf8Sequence[0];
	} 
	auto utf8Length = core::utf8::sequenceLength(utf8Sequence[0]);
	for(uint32 i = 1;i < utf8Length;i++) utf8Sequence[i] = peek(i);
	length = utf8Length;
	return core::utf8::decode(utf8Length,utf8Sequence);
}
uint32 CharSource::peekUtf8(uint32& length,int offset){
	uint8  utf8Sequence[4];
	utf8Sequence[0] = peek(offset);

	if(utf8Sequence[0] < 127){
		length = 1;
		return utf8Sequence[0];
	} 
	auto utf8Length = core::utf8::sequenceLength(utf8Sequence[0]);
	for(uint32 i = 1;i < utf8Length;i++) utf8Sequence[i] = peek(offset+i);
	length = utf8Length;
	return core::utf8::decode(utf8Length,utf8Sequence);
}
void CharSource::read(int amount,uint8* buffer){
	if(LIKELY_TRUE((src+amount) <= end)){
		if(buffer) memcpy(buffer,src,amount);
		src+=amount;
		return;
	}
	for(;;){
		auto diff = end - src;
		if(amount <= diff){
			if(buffer) memcpy(buffer,src,amount);
			src+=amount;
			break;
		}
		if(buffer){
			memcpy(buffer,src,diff);
			buffer+=diff;
		}
		refill();
		amount -= diff;
	}
}


//Move the remaining of the data to the buffer
void CharSource::moveToBuffer() {
	auto size = (size_t)(end - src);
	if(size != 0) memcpy(buffer,src,size);
	src = buffer;
	end = src+ size;
}
void CharSource::moveBuffer() {
	auto size = (size_t)(end - src);
	if(size != 0) memmove(buffer,src,size);
	src = buffer;
	end = src+ size;
}

void CharSource::refill(int lookahead){
	assert(lookahead <= kBufferSize/2);

	//No provider -> EOF
	if(!provider){
		if(lookahead > 0){
			if(!(src >= buffer && src < buffer+kBufferSize)) moveToBuffer();
		} else {
			src = buffer;
			end = src+1;
		}
		assert(src+lookahead < buffer+kBufferSize);
		*(src + lookahead) = '\0';
		return;
	}

	//If we are peeking, keep the current data, otherwise dispose of it.
	if(lookahead > 0) moveBuffer();
	else {
		src= end= buffer;
	}

	//Provider -> get data chunk by chunk
	for(;;){
		if(data.begin >= data.end) data = provider->get();

		size_t capacity = size_t((buffer + kBufferSize) - end);
		size_t dataSize = size_t(data.end - data.begin);

		if(dataSize == 0){ //EOF reached
			assert(capacity != 0);
			*end = '\0';end++;
			endOffset++;
			provider = nullptr;
			if(src + lookahead >= end){
				assert(src+lookahead < buffer+kBufferSize);
				*(src + lookahead) = '\0';
			}
			break;
		}
		if(dataSize < capacity){
			memcpy(end,data.begin,dataSize);
			data.begin+= dataSize;
			end+=dataSize;
			endOffset+=uint32(dataSize);
			continue;
		}
		else  {
			memcpy(end,data.begin,capacity);
			data.begin+= capacity;
			end+=capacity;
			endOffset+=uint32(capacity);
			break;
		}
	}
}

}