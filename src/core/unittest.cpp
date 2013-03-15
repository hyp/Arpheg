#include <string.h>
#include <string>
#include <iostream>
#include "assert.h"
#include "hash/murmur.h"
#include "typeDescriptor.h"
#include "text.h"
#include "structLayout.h"
#include "charSource.h"
#include "range.h"
#include "utf.h"
#include "bufferArray.h"
#include "bufferStack.h"
#include "bufferStringStream.h"

void testMurmurHash(){
	const char *s = "test string";
	assert(core::murmurHash32(s,strlen(s),0) == 0x159061E2);
}
void testMemory(){
	assert(core::memory::align_forward((void*)0,4) == (void*)0);
	assert(core::memory::align_forward((void*)1,4) == (void*)4);
	assert(core::memory::align_forward((void*)3,4) == (void*)4);
	assert(core::memory::align_forward((void*)4,4) == (void*)4);

	core::BufferAllocator buffer(1024);
	auto ptr = buffer.allocate(256);
	assert(buffer.size() == 256);
	assert(buffer.capacity() == 1024);
	assert(buffer.canAllocate(128));
	assert(!buffer.canAllocate(1024));
	assert(buffer.toOffset(ptr) == 0);
	assert(buffer.toPointer(0) == ptr);
	buffer.reset();
	assert(buffer.size() == 0);
}
void testUtf(){
	struct test {
		test(const char* string,uint32 codepoint){
			auto len = ((uint8)string[0]) < 128? 1: core::utf8::sequenceLength(string[0]);
			auto c = core::utf8::decode(len,(const uint8*)(string));
			assert(c == codepoint && "Invalid unicode codepoint");
			string+=len;
		}
	};

	test a("_",'_');
	test b("\xC2\xA2",0x00A2);
	test c("\xC2\xA9",0x00A9);
	test d("\xC5\x8C",0x014C);
	test e("\xC5\xBE",0x017E);
	test f("\xD0\xA0",0x0420);
	test g("\xE0\xA6\x95",0x0995);
	test h("\xF0\x9D\x90\x80",0x1D400);
}
void testRange(){
	struct Foo {
		int i;
	};
	Foo arr[] = { {0},{1},{2},{3} };
	core::Range<Foo> foos(arr,sizeof(arr)/sizeof(Foo));
	assert(foos->i == 0);
	assert((*foos).i == 0);
	assert(foos.next()->i == 1);
	assert(foos.nextOrNull()->i == 1);
	assert(foos.count() == 4);
	assert(!foos.empty());
	++foos;
	assert(foos->i == 1);
	assert((*foos).i == 1);
	assert(foos.next()->i == 2);
	++foos;
	assert(foos->i == 2);
	assert((*foos).i == 2);
	assert(foos.next()->i == 3);
	++foos;
	assert(foos->i == 3);
	assert((*foos).i == 3);
	assert(foos.nextOrNull() == nullptr);
	assert(!foos.empty());
	++foos;
	assert(foos.empty());
}
void testBufferAllocator(){
	uint8 memory[128];
	core::BufferAllocator buffer(core::Bytes(memory,memory+ sizeof(memory)));
	assert(buffer.allocate(32,0) == memory);
	assert(buffer.allocate(32,0) == memory+32);

	core::BufferAllocator buffer2(core::Bytes(memory,memory + sizeof(memory)),core::BufferAllocator::GrowOnOverflow);
	assert(buffer2.allocate(128,0) == memory);
	assert(buffer2.allocate(128,0) != nullptr);
}
void testBufferArray(){
	core::BufferAllocator buffer(1024);

	using namespace core::bufferArray;

	assert(length<int>(buffer) == 0);
	add<int>(buffer,69);
	assert(length<int>(buffer) == 1);
	assert(front<int>(buffer) == 69);
	assert(back<int>(buffer) == 69);
	assert(nth<int>(buffer,0) == 69);
	add<int>(buffer,42);
	assert(length<int>(buffer) == 2);
	assert(front<int>(buffer) == 69);
	assert(back<int>(buffer) == 42);
	assert(nth<int>(buffer,0) == 69);
	assert(nth<int>(buffer,1) == 42);
	int nobNews[] = {1,2,3};
	add<int>(buffer,nobNews,3);
	assert(length<int>(buffer) == 5);
	assert(back<int>(buffer) == 3);
	assert(nth<int>(buffer,2) == 1);
	assert(nth<int>(buffer,3) == 2);

	assert(indexOf<int>(buffer,42) == 1);
	assert(indexOf<int>(buffer,666) == -1);
	add<int>(buffer,21);
	add<int>(buffer,21);
	assert(indexOf<int>(buffer,21) == 5);
	assert(lastIndexOf<int>(buffer,21) == 6);
	replace<int>(buffer,21,0);
	assert(indexOf<int>(buffer,21) == -1);
	assert(indexOf<int>(buffer,0) != -1);

	removeLast<int>(buffer);
	assert(length<int>(buffer) == 6);
	removeLast<int>(buffer);
	assert(length<int>(buffer) == 5);
	assert(back<int>(buffer) == 3);
}
void testBufferStack(){
	core::BufferAllocator buffer(1024);

	using namespace core::bufferStack;
	assert(isEmpty<int>(buffer));
	push<int>(buffer,4);
	assert(!isEmpty<int>(buffer));
	assert(peek<int>(buffer) == 4);
	assert(!isEmpty<int>(buffer));
	assert(pop<int>(buffer) == 4);
	assert(isEmpty<int>(buffer));
}
void testBufferStringStream(){
	core::BufferAllocator buffer(1024);

	using namespace core::bufferStringStream;
	assert(strcmp(asCString(buffer),"") == 0);
	buffer<<'X';
	assert(strcmp(asCString(buffer),"X") == 0);
	buffer<<"Test";
	assert(strcmp(asCString(buffer),"XTest") == 0);
	buffer<<1;
	assert(strcmp(asCString(buffer),"XTest1") == 0);
	buffer<<2.5;
	assert(strcmp(asCString(buffer),"XTest12.5") == 0);
	printf(buffer,"!%d yes",69);
	assert(strcmp(asCString(buffer),"XTest12.5!69 yes") == 0);
}
void testCharSource(){
	io::CharSource source1("Test");
	assert(source1.position() == 0);
	assert(source1.consume() == 'T');
	assert(source1.position() == 1);
	assert(source1.consume() == 'e');
	assert(source1.position() == 2);
	assert(source1.peek() == 's');
	assert(source1.peek(0) == 's');
	assert(source1.peek(1) == 't');
	assert(source1.position() == 2);
	assert(source1.peek(2) == '\0');
	assert(source1.peek(3) == '\0');
	assert(source1.consume() == 's');
	assert(source1.position() == 3);
	assert(source1.peek() == 't');
	assert(source1.consume() == 't');
	assert(source1.consume() == '\0');

	class Provider: public io::DataProvider {
	public:
		const char* str;
		Provider(const char* s) : str(s) {}
		core::Bytes get(){
			if(*str == '\0'){
				return core::Bytes(nullptr,nullptr);
			} else{
				auto p = str;str++;
				return core::Bytes((void*)p,1);
			}
		}
	};
	std::string str = "Test string";
	Provider provider(str.c_str());
	io::CharSource source2(&provider);
	assert(source2.position() == 0);
	for(size_t i =0;i < str.size(); i++){
		assert(source2.position() == i);
		assert(source2.peek() == str[i]);
		if(i < (str.size()-1))
			assert(source2.peek(1) == str[i+1]);
		assert(source2.consume() == str[i]);
	}
	assert(source2.peek() == '\0');
	assert(source2.consume() == '\0');

	io::CharSource source3("Test");
	assert(source3.peek() == 'T');
	source3.skip(2);
	assert(source3.peek() == 's');
	source3.skip(1);
	assert(source3.peek() == 't');
	source3.skip(2);
	assert(source3.peek() == '\0');

	io::CharSource source4("Test4");
	uint8 buffer[6]= { 0 };
	source4.read(sizeof(buffer)-1,buffer);
	assert(strcmp((char*)buffer,"Test4") == 0);
	assert(source4.peek() == '\0');

	io::CharSource source5("U\xC2\xA2\xE0\xA6\x95");
	uint32 len = 0;
	assert(source5.peekUtf8(len) == 'U');
	assert(len == 1);
	source5.skip(len);
	assert(source5.peekUtf8(len) == 0x00A2);
	assert(len == 2);
	source5.skip(len);
	assert(source5.peekUtf8(len) == 0x0995);
	assert(len == 3);
	source5.skip(len);
}
static core::Bytes bytes(const char* text){
	return core::Bytes((void*)text,strlen(text));
}
inline bool operator == (core::Bytes str,const char* text){
	if(strlen(text) != str.length()) return false;
	return str.empty() || memcmp(str.begin,text,str.length()) == 0;
}
void testText(){
	using namespace core::text;

	assert(trimFront(bytes("Test")) == "Test");
	assert(trimFront(bytes("Test  ")) == "Test  ");
	assert(trimFront(bytes(" Test  ")) == "Test  ");

	assert(trim(bytes("Test")) == "Test");
	assert(trim(bytes("Test  ")) == "Test");
	assert(trim(bytes(" Test  ")) == "Test");

	auto src = bytes("Hello world\r\nLine 2\n4 -123");
	assert(parseLine(src) == "Hello world");
	assert(parseLine(src) == "Line 2");
	assert(parseInt32(src) == 4);
	src.begin++;
	assert(parseInt32(src) == -123);
	assert(src.empty());

	auto src2 = bytes(
		"\tTest string\n"
		"\tHello\n"
		"\n"
		"\tWorld\n"
		"\t\tTest\n"
		"\t\n");
	auto foo = parseIndentedLines(src2);
	assert(foo ==
		"\tTest string\n"
		"\tHello\n"
		"\n"
		"\tWorld\n"
		"\t\tTest\n"
		"\t");

	src2 = bytes(
		"\tTest string\n"
		"\tHello\n"
		"\n"
		"\tWorld\n"
		"\t\tTest\n"
		"\t");
	foo = parseIndentedLines(src2);
	assert(foo ==
		"\tTest string\n"
		"\tHello\n"
		"\n"
		"\tWorld\n"
		"\t\tTest");

	src2 = bytes(
		"\tDescription\n"
		"\n"
		"\tGarage band\n"
		"-\n");
	foo = parseIndentedLines(src2);
	assert(foo == 	
		"\tDescription\n"
		"\n"
		"\tGarage band");

	std::string sample = "Hello world 12\r\n \n\t\n  Foo fighters!\r\n";

	int lineNo = 0;
	for(Lines lines(core::Bytes((void*)sample.c_str(),sample.length()));!lines.empty();lineNo++){
		auto line = lines.getLineTrimmedSkipEmpty();
		std::string text((const char*) line.begin,(const char*) line.end);
		if(lineNo == 0) assert(text == "Hello world 12");
		else assert(text == "Foo fighters!");
	}
	assert(lineNo == 2);
}
void testTypeDescriptor(){
	core::TypeDescriptor descriptor;
	descriptor.id = core::TypeDescriptor::TInt32;
	descriptor.count = 1;
	assert(descriptor.size() == sizeof(int32));
	descriptor.count = 4;
	assert(descriptor.size() == sizeof(int32)*4);
	descriptor.id = core::TypeDescriptor::TUint16;
	assert(descriptor.size() == sizeof(uint16)*4);

	descriptor.id = core::TypeDescriptor::TFloat;
	assert(descriptor.size() == sizeof(float)*4);
	descriptor.id = core::TypeDescriptor::TDouble;
	assert(descriptor.size() == sizeof(double)*4);
}
void testStructLayout(){
	STRUCT_PREALIGN(16) struct floatx4 {
		float v[4];
	} STRUCT_POSTALIGN(16);
	assert(alignof(floatx4) == 16);
}

void testCore(){
	testMurmurHash();
	testMemory();
	testText();
	testTypeDescriptor();
	testStructLayout();
	testCharSource();
	testBufferAllocator();
	testBufferArray();
	testBufferStack();
	testBufferStringStream();
	testUtf();
}

int main(){
	testCore();
	return 0;
}