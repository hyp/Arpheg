#include <string.h>
#include "../../core/memory.h"
#include "../../services.h"
#include "../../rendering/opengl/gl.h"
#include "../data.h"

#include "preprocess.h"

namespace data {
namespace shader {

Preprocessor::Preprocessor(core::Allocator* allocator){
	src = nullptr;
	this->allocator = allocator;
}
Preprocessor::~Preprocessor(){
	if(src) allocator->deallocate(src);
	src = nullptr;
}
core::Bytes Preprocessor::preprocess(core::Bytes source){

#ifdef ARPHEG_RENDERING_GL

	auto string = glGetString(GL_SHADING_LANGUAGE_VERSION);
	char versionHeader[] = { "#version xxx core\n" };
	versionHeader[9]=string[0];
	versionHeader[10]=string[2];
	versionHeader[11]=string[3];
	auto versionHeaderLength = strlen(versionHeader);

	if(src) allocator->deallocate(src);
	auto srcLen = versionHeaderLength+source.length();
	src = (uint8*)allocator->allocate(srcLen+1);
	memcpy(src,versionHeader,versionHeaderLength);
	memcpy(src+versionHeaderLength,source.begin,source.length());
	src[srcLen] = '\0';

	auto logger = services::logging();
	if(logger->priority() <= application::logging::Trace){
		logger->trace("Final GLSL shader:");
		logger->trace((char*)src);
	}

	return core::Bytes(src,srcLen);
#else

	return source;
#endif
}
} }

/*
namespace intermediate {
namespace shader {

class Preprocessor {
public:
	virtual const char* replace(core::Bytes id){
		return nullptr;
	}
	void preprocess(core::Bytes source,core::BufferAllocator& destination,char id = '$'){
		using namespace core::bufferStringStream;
	
		auto begin = source.begin;
		auto portionBegin = begin;//start of the raw segmented
		auto end = source.end;

	#define EMIT_RAW if(begin > portionBegin) destination<<core::Bytes(portionBegin,begin)

		for(;begin < end;++begin){
			//Matches a definition?
			if(begin[0] == id){
				EMIT_RAW;
				++begin;
				//parse identifier
				core::Bytes runningSource(begin,source.end);
				auto id = text::Parser::identifier(runningSource);
				portionBegin = begin = runningSource.begin;
				if(auto replacement = replace(id)) destination<<replacement;
				else ;//TODO error
			}
		}
		EMIT_RAW;

	#undef EMIT_RAW
	}
};

enum Command {
	GLSL_SHADER,
	HLSL_SHADER,
	VERTEX_LAYOUT,
};

Parser::Parser(core::Allocator* allocator) : sourceBuffer(1024*4,allocator,core::BufferAllocator::GrowOnOverflow) {
}
void Parser::parse(core::Bytes bytes,rendering::Service* renderer) {
	using namespace core::bufferStringStream;

	this->renderer = renderer;

	auto string = glGetString(GL_SHADING_LANGUAGE_VERSION);
	char versionHeader[] = { "#version xxx core" };
	versionHeader[9]=string[0];
	versionHeader[10]=string[2];
	versionHeader[11]=string[3];
	this->versionHeader = versionHeader;

	shaderCount = 0;
	shaderType = 0;
	text::Parser::parse(bytes);
	if(shaderCount < 2){
		formatter.allocator<<"Not enough shaders";
		error();
		return;
	}
	//pipeline = renderer->create(rendering::VertexDescriptor::positionAs3Float_normalAs3Float_texcoordAs2Float(),shaders,shaderCount);
}
Parser::SubDataHandling Parser::handleSubdata(core::Bytes id){
	using namespace core::bufferStringStream;

	if(match(id,"glsl.")){
		command = GLSL_SHADER;
		shaderType = rendering::Shader::MaxTypes;
		if(compare(id,"vertex")) shaderType = rendering::Shader::Vertex;
		else if(compare(id,"pixel")) shaderType = rendering::Shader::Pixel;
		else if(compare(id,"geometry")) shaderType = rendering::Shader::Geometry;
		return shaderType != rendering::Shader::MaxTypes? Parser::SubDataCustomBlock : Parser::SubDataNone;
	} else if(compare(id,"vertex.layout")) {
		command = VERTEX_LAYOUT;
		return Parser::SubDataCustomBlock;
	}
	return Parser::SubDataNone;
}
void Parser::subdata(core::Bytes subdata){
	using namespace core::bufferStringStream;
	if(command != GLSL_SHADER) return;

	class GlslPreprocessor: public Preprocessor {
	public:
		const char* glslVersionHeader;
		const char* vertexLayout;
		const char* replace(core::Bytes id){
			if(compare(id,"glsl.version.header")) return glslVersionHeader;
			else if(compare(id,"vertex.layout")) return vertexLayout;
			return nullptr;
		}
	};
	GlslPreprocessor preprocessor;
	preprocessor.glslVersionHeader = versionHeader;
	preprocessor.vertexLayout = "in vec3 position;\nin vec3 normal;\nin vec2 texcoord;";
	preprocessor.preprocess(subdata,sourceBuffer);

	auto logger = services::logging();
	if(logger->priority() <= application::logging::Trace){
		logger->trace("Final GLSL shader:");
		logger->trace(asCString(sourceBuffer));
	}

	shaders[shaderCount] = renderer->create(rendering::Shader::Type(shaderType),(const char*)sourceBuffer.bufferBase(),sourceBuffer.size());
	sourceBuffer.reset();
	shaderCount++;
}

} } }*/