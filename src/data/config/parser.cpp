//TODO config files.

#include "../../core/assert.h"
#include "../../core/io.h"
#include "parser.h"

namespace data {
namespace config {

Parser::Parser(){
	setFunc = nullptr;
	endFunc = nullptr;
}

void Parser::set(core::Bytes bytes){
	if(setFunc)
		setFunc(this,bytes);
}

void loadFromTextFile(const char* filename) {
	io::Data file(filename);
	Parser parser;
	parser.parse(core::Bytes(file.begin,file.size));
}

} }