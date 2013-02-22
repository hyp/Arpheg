#include <string.h>
#include "assert.h"
#include "text.h"

namespace core {
namespace text {

Tokenizer::Tokenizer(Bytes data,uint32 modes): data_(data),mode(modes) {
}
Tokenizer::Tokenizer(const char* str,uint32 modes): data_(core::Bytes((uint8*)str,strlen(str))),mode(modes) {
}
void Tokenizer::trim(){
	for(;tokenStart < tokenEnd;tokenStart++){
		if(*tokenStart != ' ' && *tokenStart != '\t') break;
	}
	for(tokenEnd--;tokenEnd>=tokenStart;tokenEnd--){
		if(*tokenEnd != ' ' && *tokenEnd != '\t') break;
	}
	tokenEnd++;
}
void Tokenizer::getLineToken() {
start:
	tokenStart = data_.begin;
	tokenEnd = data_.end;
	for(;data_.begin < data_.end;data_.begin++){
		auto firstCharacter = *data_.begin;
		if(firstCharacter >= 0xA && firstCharacter <= 0xD){
			tokenEnd = data_.begin;
			data_.begin++;
			if(firstCharacter == 0xD && data_.begin < data_.end && *data_.begin == 0xA)
				data_.begin++;//CR+LF
			break;
		}
	}
	auto untrimmedEnd = tokenEnd;
	if(mode & Trim) trim();
	if(mode & SkipEmpty){
		if(tokenStart == tokenEnd && untrimmedEnd != data_.end) goto start;
	}
}
void Tokenizer::skipSpaces() {
	for(;data_.begin < data_.end;data_.begin++){
		if(*data_.begin != ' ' && *data_.begin != '\t') break;
	}
}
void Tokenizer::getAsciiIdentifier() {
	tokenStart = data_.begin;
	tokenEnd = data_.end;
	for(;data_.begin < data_.end;data_.begin++){
		auto firstCharacter = *data_.begin;
		if( (firstCharacter >= 'a' && firstCharacter <= 'z') ||
			(firstCharacter >= 'A' && firstCharacter <= 'Z') ||
			(firstCharacter == '_') ){
				continue;
		}
		tokenEnd = data_.begin;
		break;
	}
}
static bool compare(Bytes token,const char* str){
	for(;*str!='\0';str++,token.begin++){
		if(token.begin >= token.end) return false;
		if((char)*token.begin != *str) return false;
	}
	return token.begin >= token.end;
}
bool Tokenizer::matchAsciiIdentifier(const char* token) {
	return compare(core::Bytes(tokenStart,tokenEnd),token);
}
bool Tokenizer::matchToken(const char* token) {
	if(mode & Trim) skipSpaces();
	tokenStart = data_.begin;
	tokenEnd = data_.begin;
	for(;*token!='\0';tokenEnd++,token++){
		if(tokenEnd >= data_.end) return false;
		if((char)*tokenEnd != *token) return false;
	}
	data_.begin = tokenEnd;
	return true;
}
void Tokenizer::expectToken(const char* token) {
	if(!matchToken(token))
		assert(false);
}
int  Tokenizer::expectInteger() {
	if(mode & Trim) skipSpaces();
	tokenStart = data_.begin;
	tokenEnd   = data_.end;
	int value = 0;
	int sign  = 1;
	if(data_.begin < data_.end){
		if(*data_.begin == '-'){
			sign = -1;
			data_.begin++;
		}
	}
	for(;data_.begin < data_.end;data_.begin++){
		auto firstCharacter = *data_.begin;
		if(firstCharacter >= '0' && firstCharacter <= '9'){
			value = value*10 + (int)(firstCharacter - '0');
			continue;
		}
		tokenEnd = data_.begin;
		break;
	}
	return sign == -1? -value:value;
}
float Tokenizer::expectFloat(){
	if(mode & Trim) skipSpaces();
	tokenStart = data_.begin;
	tokenEnd   = data_.end;
	float value = 0.0f;
	float iteration = 0.1f;
	bool plus = true;
	bool integer = true;
	if(data_.begin < data_.end){
		if(*data_.begin == '-'){
			plus = false;
			data_.begin++;
		}
	}
	for(;data_.begin < data_.end;data_.begin++){
		auto firstCharacter = *data_.begin;
		if(firstCharacter >= '0' && firstCharacter <= '9'){
			if(integer)
				value = value*10.0f + (float)(firstCharacter - '0');
			else {
				value = value + iteration*(float)(firstCharacter - '0');
				iteration*=0.1f;
			}
			continue;
		}
		else if(integer && firstCharacter == '.'){
			integer=false;
			continue;
		}
		tokenEnd = data_.begin;
		break;
	}
	return plus? value:-value;
}
void Tokenizer::readString(char* buffer,int bufferLength,char end) {
	int i = 0;
	for(;data_.begin < data_.end && i < (bufferLength-1);data_.begin++,i++){
		auto firstCharacter = *data_.begin;
		if((char)firstCharacter == end){
			data_.begin++;
			break;
		} else 
			buffer[i] = (char)firstCharacter;
	}
	buffer[i] = '\0';
	tokenEnd = data_.begin;
}

Lines::Lines(Bytes data,uint32 modes) : Tokenizer(data,modes) {
}

}
}