#include <ctype.h>
#include <string.h>
#include "../../core/text.h"
#include "../../services.h"
#include "parser.h"

namespace data {
namespace text {

bool Parser::compare(core::Bytes id,const char* str) {
	auto src = id.begin;
	for(;str[0] != '\0' && src<id.end;src++,str++){
		if(*src != *str) return false;
	}
	return str[0] == '\0' && src==id.end;
}
bool Parser::match(core::Bytes& id,const char* str) {
	auto src = id.begin;
	for(;str[0] != '\0' && src<id.end;src++,str++){
		if(*src != *str) return false;
	}
	if(str[0] == '\0'){
		id.begin = src;
		return true;
	}
	return false;
}
core::Bytes Parser::identifier(core::Bytes& data){
	core::Bytes id = data;
	auto src = data.begin;
	for(;src < data.end;src++){
		if(!isalnum(*src) && *src != '.'){
			id.end = src;
			break;
		}
	}
	data.begin = src;
	return id;
}

#define ERROR_UNRECOGNIZED_ID(id)\
	formatter.allocator<<"The identifier '"<<id<<"' wasn't recongnized!"; \
	error();


Parser::Parser() : data(nullptr,nullptr){
	arrayStorageLength= 0;
	setResult_ = false;
}
void Parser::error(){
	using namespace core::bufferStringStream;

	Formatter fmt;
	fmt.allocator<<"Parsing error: "<<asCString(formatter.allocator);
	services::logging()->error(asCString(fmt));
	formatter.allocator.reset();
}
void Parser::get(core::Bytes id) {
}
void Parser::set(core::Bytes id) {
}
Parser::SubDataHandling Parser::handleSubdata(core::Bytes id) {
	return SubDataNone;
}
void Parser::subdata(core::Bytes subdata) {
}
void Parser::endSubdata() {

}
void Parser::typecheck(uint32 type) {
	using namespace core::bufferStringStream;

	auto t = currentVal->tag;
	if(t != type){
		formatter.allocator<<"Type mismatch";
		error();
	}
}
void Parser::typecheck(uint32 type,uint32 count) {
	using namespace core::bufferStringStream;
	if(currentVal->tag == TaggedValue::Array){
		if(currentVal->value.array.length == count){
			for(uint32 i = 0;i < count;++i){
				TaggedValue& element = currentVal->value.array.begin[i];
				if(element.tag != type){
					goto Err;
				}
			}
			return;
		}
	}

Err:
	formatter.allocator<<"Array type mismatch";
	error();
}
void Parser::typecheck(uint32 types[],uint32 count) {
	using namespace core::bufferStringStream;
	if(currentVal->tag == TaggedValue::Array){
		if(currentVal->value.array.length == count){
			for(uint32 i = 0;i < count;++i){
				TaggedValue& element = currentVal->value.array.begin[i];
				if(element.tag != types[i]){
					goto Err;
				}
			}
			return;
		}
	}

Err:
	formatter.allocator<<"Array type mismatch";
	error();
}

void Parser::string(const char* str){
	getResult_.tag = TaggedValue::String;
	getResult_.value.string.begin = (uint8*) str;
	getResult_.value.string.end   = ((uint8*) str) + strlen(str);
}
void Parser::number(float n){
	getResult_.tag = TaggedValue::Number;
	getResult_.value.number = n;
}
		
core::Bytes Parser::string(){
	using namespace core::bufferStringStream;

	auto t = currentVal->tag;
	if(t != TaggedValue::String){
		formatter.allocator<<"Type mismatch";
		error();
		return core::Bytes(nullptr,nullptr);
	}
	setResult_ = true;
	return core::Bytes(currentVal->value.string.begin,currentVal->value.string.end);
}
uint32 Parser::strings(core::Bytes* dest,uint32 max) {
	using namespace core::bufferStringStream;
	if(currentVal->tag == TaggedValue::Array){
		if(currentVal->value.array.length <= max){
			for(uint32 i = 0;i < currentVal->value.array.length;++i){
				TaggedValue& element = currentVal->value.array.begin[i];
				if(element.tag != TaggedValue::String){
					goto Err;
				}
			}
		} else goto Err;
	} else goto Err;
	for(uint32 i = 0;i < currentVal->value.array.length;++i){
		TaggedValue& element = currentVal->value.array.begin[i];
		dest[i].begin= element.value.string.begin;
		dest[i].end  = element.value.string.end;
	}	
	
	setResult_ = true;
	return currentVal->value.array.length;
Err:
	formatter.allocator<<"Array type mismatch";
	error();
	return 0;
}
float Parser::number(){
	using namespace core::bufferStringStream;

	auto t = currentVal->tag;
	if(t != TaggedValue::Number){
		formatter.allocator<<"Type mismatch";
		error();
		return 0.0f;
	}
	setResult_ = true;
	return currentVal->value.number;
}
bool  Parser::boolean() {
	auto n = number();
	return n != 0.0;
}
vec2f Parser::vec2(){
	vec2f result;
	typecheck(TaggedValue::Number,2);
	result.x = currentVal->value.array.begin[0].value.number;
	result.y = currentVal->value.array.begin[1].value.number;
	setResult_ = true;
	return result;
}
vec3f Parser::vec3(){
	vec3f result;
	typecheck(TaggedValue::Number,3);
	result.x = currentVal->value.array.begin[0].value.number;
	result.y = currentVal->value.array.begin[1].value.number;
	result.z = currentVal->value.array.begin[2].value.number;
	setResult_ = true;
	return result;
}
vec4f Parser::vec4(){
	vec4f result;
	typecheck(TaggedValue::Number,4);
	result.x = currentVal->value.array.begin[0].value.number;
	result.y = currentVal->value.array.begin[1].value.number;
	result.z = currentVal->value.array.begin[2].value.number;
	result.w = currentVal->value.array.begin[3].value.number;
	setResult_ = true;
	return result;
}

//Parsing
TaggedValue Parser::value(){
	using namespace core::text;
	using namespace core::bufferStringStream;

	TaggedValue result;
	data = trimFront(data);
	if(isdigit(data.begin[0]) || data.begin[0]=='-' || data.begin[0] == '.'){
		result.tag = TaggedValue::Number;
		result.value.number = parseFloat(data);
	} else if(data.begin[0] == '\'' || data.begin[0] == '"'){
		auto terminator = data.begin[0];
		data.begin++;//skip '
		result.tag = TaggedValue::String;
		auto str = parseUntil(data,terminator);
		result.value.string.begin = str.begin;
		result.value.string.end   = str.end;
		data.begin++;//skip '
	} else {
		auto id = identifier(data);
		if(id.empty()){
			formatter.allocator<<"Expected a value";
			error();
			result.tag = TaggedValue::Nothing;
		} else {
			getResult_.tag = TaggedValue::Nothing;
			get(id);
			if(getResult_.tag == TaggedValue::Nothing){
				ERROR_UNRECOGNIZED_ID(id);
			}
			result = getResult_;
		}
	}
	return result;
}
TaggedValue Parser::values() {
	using namespace core::text;
	using namespace core::bufferStringStream;

	for(arrayStorageLength = 0;;){
		arrayStorage[arrayStorageLength] = value();
		arrayStorageLength++;
		data = trimFront(data);
		if(data.empty() || data.begin[0] != ',') break;
		data.begin++;//skip ','
		if(arrayStorageLength >= TaggedArray::kMaxElements){
			formatter.allocator<<"This inplace array is too long";
			error();
			arrayStorageLength -= 1;
		}
	}
	/*if(arrayStorageLength < 2){
		formatter.allocator<<"Not an array";
		error();
	}*/
	TaggedValue val;
	val.tag = TaggedValue::Array;
	val.value.array.begin = arrayStorage;
	val.value.array.length = arrayStorageLength;
	return val;
}
void Parser::statement() {
Start:
	using namespace core::text;
	using namespace core::bufferStringStream;

	data = trimFront(data);
	if(matchNewline(data)) goto Start;
	bool expectNewline = true;

	//identifier
	auto id = identifier(data);
	if(id.empty()){
		if(!data.empty()){
			formatter.allocator<<"Expected a valid identifier";
			error();
		}
		return;
	}
	//postfix ':' or postfix '='
	data = trimFront(data);
	if(data.empty()) goto ExpError;
	if(data.begin[0] == ':'){
		data.begin++;
		data = trimFront(data);
		if(matchNewline(data)){
			//Embedded array
			auto handling = handleSubdata(id);
			if(handling == SubDataCustomBlock){
				subdata(parseIndentedLines(data));
			} else if(handling == SubDataObject){
				auto subdata = parseIndentedLines(data);
				auto dat = data;
				parse(subdata);
				data = dat;
				endSubdata();
			} else {
				ERROR_UNRECOGNIZED_ID(id);
			}
			expectNewline = false;
		} else {
			//One line inplace array
			setResult_ = false;
			auto val = values();
			if(val.tag){
				currentVal=&val;
				set(id);
				if(!setResult_){
					ERROR_UNRECOGNIZED_ID(id);
				}
			}
		}		
	} else if(data.begin[0] == '='){
		data.begin++;
		setResult_ = false;
		auto val = value();
		if(val.tag){
			currentVal=&val;
			set(id);
			if(!setResult_){
				ERROR_UNRECOGNIZED_ID(id);
			}
		}
	} else {
		goto ExpError;
	}
	//Expect newline
	if(!expectNewline) return;
	data = trimFront(data);
	if(!matchNewline(data)){
		if(!data.empty()) {
			goto NlError;
		}
	}
	return;
ExpError:
	formatter.allocator<<"Expected ':' or '=' after '"<<id<<"'";
	error();
	return;
NlError:
	formatter.allocator<<"Expected newline after this statement";
	error();
	return;
}
void Parser::parse(core::Bytes dat_){
	using namespace core::text;
	using namespace core::bufferStringStream;

	for(data = trimFront(dat_);!data.empty();){
		statement();
	}
}

#undef ERROR_UNRECOGNIZED_ID

} }