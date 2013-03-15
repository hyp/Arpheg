#include "../../core/assert.h"
#include "../../core/io.h"
#include "parser.h"

namespace data {
namespace config {

Parser::Parser(){
	setFunc = nullptr;
	endFunc = nullptr;
}
/*
Parser::SubDataHandling Parser::handleSubdata(core::Bytes id) {
	if(setFunc) return SubDataNone;
	data = nullptr;
	if(compare(id,"mesh")){ //3D mesh resource
		setFunc = meshSet;
		endFunc = meshDone;
		return SubDataObject;
	} else if(compare(id,"material")){ //Rendering material resource

	} else if(compare(id,"pipeline")){ //Shading pipeline resource
		setFunc = pipelineSet;
		endFunc = pipelineDone;
		return SubDataObject;
	} else if(compare(id,"texture2DArray")){
		//setFunc = texture2DArraySet;
		//endFunc = texture2DArrayDone;
		return SubDataObject;
	} else if(compare(id,"font")){
		
	} else if(compare(id,"sprite")){
		
	} else if(compare(id,"text")){
		
	}
	return SubDataNone;
}
void Parser::endSubdata(){
	if(endFunc) endFunc(this);
	endFunc = nullptr;
	setFunc = nullptr;
}

*/
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