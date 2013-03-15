#include <string.h>
#include <stdio.h>
#include "../data.h"
#include "../../core/assert.h"
#include "../../core/bufferStringStream.h"
#include "parser.h"

inline bool operator == (core::Bytes str,const char* text){
	if(strlen(text) != str.length()) return false;
	return str.empty() || memcmp(str.begin,text,str.length()) == 0;
}
static core::Bytes bytes(const char* text){
	return core::Bytes((void*)text,strlen(text));
}

void testParser(){
	using namespace data::text;

	class TestParser: public Parser {
	public:
		float y;
		core::Bytes subblock;
		core::Bytes str;
		vec2f position;
		int errorCount;
		bool objState;
		float objY;
		
		uint32 nameCount;
		core::Bytes* names;
		uint8 nameStorage[256];


		TestParser() : errorCount(0),subblock(nullptr,nullptr),str(nullptr,nullptr),objState(false) {
			names = (core::Bytes*) nameStorage;
		}
		void get(core::Bytes id){
			if(compare(id,"x"))
				number(2.0f);
		}
		void set(core::Bytes id){
			if(objState){
				if(compare(id,"y")){
					objY = number();
				}
			}
			if(compare(id,"y"))
				y = number();
			else if(compare(id,"str"))
				str = string();
			else if(compare(id,"position"))
				position = vec2();
			else if(compare(id,"names"))
				nameCount = strings(names,4);
			/*else if(compare(id,"ystr")){
				uint32 ts[] = {TaggedValue::Number,TaggedValue::String};
				typecheck(ts,2);
				y   = obj.value.array.begin[0].value.number;
				str = core::Bytes(obj.value.array.begin[1].value.string.begin,obj.value.array.begin[1].value.string.end);
			}*/
		}
		void endSubdata(){
			objState = false;
		}
		SubDataHandling handleSubdata(core::Bytes id){
			if(compare(id,"block")){
				assert(match(id,"bl"));
				assert(compare(id,"ock"));
				return SubDataCustomBlock;
			}
			else if(compare(id,"object")){
				objState = true;
				return SubDataObject;
			}
			return SubDataNone;
		}
		void subdata(core::Bytes subdata){
			subblock = subdata;
		}
		void error(){
			using namespace core::bufferStringStream;
			printf("%s\n",asCString(formatter.allocator));
			errorCount++;
		}
	};

	TestParser parser;
	parser.parse(bytes("y = 1"));
	assert(parser.y == 1.0);
	assert(parser.errorCount == 0);
	parser.parse(bytes("y = -3.25"));
	assert(parser.y == -3.25);
	assert(parser.errorCount == 0);
	parser.parse(bytes("y = x"));
	assert(parser.y == 2.0);
	assert(parser.errorCount == 0);
	parser.parse(bytes("y = 1 \n\n\n y = 2 \n y = 3"));
	assert(parser.y == 3.0);
	assert(parser.errorCount == 0);

	parser.parse(bytes(
		"y = 22 \n"
		"block:\n"
		"\tdata\n"
		"y = x"));
	assert(parser.y == 2.0);
	assert(parser.errorCount == 0);
	assert(parser.subblock == "\tdata");

	parser.parse(bytes("position: 42,69"));
	assert(parser.position.x == 42.0);
	assert(parser.position.y == 69.0);
	assert(parser.errorCount == 0);
	
	parser.parse(bytes("position: 42,69\n\nposition: -2.0, 4.5\n"));
	assert(parser.position.x == -2.0);
	assert(parser.position.y == 4.5);
	assert(parser.errorCount == 0);

	parser.parse(bytes("str = 'Hello world!'"));
	assert(parser.str == "Hello world!");
	assert(parser.errorCount == 0);

	parser.parse(bytes("str = \"Test\""));
	assert(parser.str == "Test");
	assert(parser.errorCount == 0);
	
	parser.parse(bytes("names: 'A', 'B', 'C'"));
	assert(parser.nameCount == 3);
	assert(parser.names[0] == "A");
	assert(parser.names[1] == "B");
	assert(parser.names[2] == "C");
	assert(parser.errorCount == 0);

	/*parser.parse(bytes("ystr:x , 'ok'"));
	assert(parser.y == 2.0);
	assert(parser.str == "ok");
	assert(parser.errorCount == 0);*/

	parser.parse(bytes("object:\n"
		"\ty = 31\n"
		"y = 13"));
	assert(parser.y == 13.0);
	assert(parser.objY == 31.0);
	assert(parser.errorCount == 0);
}

int main(){
	testParser();
	return 0;
}
