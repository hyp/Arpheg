#pragma once

#include "bytes.h"

namespace core {
namespace text {

struct Tokenizer {
	enum {
		Trim = 1,
		SkipEmpty = 2,
	};
	Tokenizer(Bytes data,uint32 modes = 0);
	Tokenizer(const char* str,uint32 modes = 0);
protected:
	Bytes data_;
	uint8 *tokenStart,*tokenEnd;
	uint32 mode;
	void trim();


public:
	inline bool empty(){
		return data_.begin >= data_.end;
	}
	inline Bytes& data(){ return data_; }

	void getLineToken();
	void skipSpaces();
	void getAsciiIdentifier();//[a-zA-Z_]
	bool matchAsciiIdentifier(const char* token);
	bool matchToken(const char* token);
	void expectToken(const char* token);
	int  expectInteger();
	float expectFloat();
	void readString(char* buffer,int bufferLength,char end);
};
struct Lines: Tokenizer {
	Lines(Bytes data,uint32 modes = 0);
	inline Bytes getLine() {
		getLineToken();
		return Bytes(tokenStart,tokenEnd);
	}
private:
	Lines(const Lines& other) : Tokenizer(Bytes(nullptr,(size_t)0)) {}
};

}
}