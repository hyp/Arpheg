#pragma once

#include "bytes.h"

namespace core {
namespace text {

Bytes trimFront(Bytes string);
Bytes trim(Bytes string);

bool  matchNewline(Bytes& string);
inline bool  peekNewline(Bytes string){
	return !string.empty() && (string.begin[0] >= 0xA && string.begin[0] <= 0xD);
}
Bytes parseLine(Bytes& string);
Bytes parseIndentedLines(Bytes& string);
int32 parseInt32(Bytes& string);
float parseFloat(Bytes& string);
Bytes parseUntil(Bytes& string,char  terminator);
Bytes parseAsciiUnderscore(Bytes& string);
void  skipSpaces(Bytes& string);


struct Lines: Bytes {
	Lines(Bytes data) : Bytes(data) {}
	inline Bytes getLine();
	inline Bytes getLineTrimmed();
	inline Bytes getLineTrimmedSkipEmpty();
};
inline Bytes Lines::getLine() { return parseLine(*this); }
inline Bytes Lines::getLineTrimmed() { return trim(parseLine(*this)); }
inline Bytes Lines::getLineTrimmedSkipEmpty() {
start:
	auto line = trim(parseLine(*this));
	if(line.empty() && !empty()) goto start;
	return line;
}

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

} }