#pragma once

#include "types.h"

namespace core {

	struct Bytes {
		uint8 *begin,*end;
		
		inline Bytes(void* data,size_t size);
		inline Bytes(void* data,void* dataEnd);
		inline bool empty() const;
		inline size_t length() const;
	};

	inline Bytes::Bytes(void* data,size_t size){
		begin = (uint8*) data;
		end = begin + size;
	}
	inline Bytes::Bytes(void* data,void* dataEnd){
		begin = (uint8*) data;
		end = (uint8*) dataEnd;
	}
	inline bool Bytes::empty()    const { return begin >= end; }
	inline size_t Bytes::length() const { return size_t(end - begin); }
}