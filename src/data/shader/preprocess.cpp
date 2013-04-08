#include <string.h>
#include "../../core/memory.h"
#include "../../core/bufferStringStream.h"
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

	using namespace core::bufferStringStream;
	Formatter defines;
#ifndef ARPHEG_RENDERING_GLES
	printf(defines.allocator,"#version %c%c%c core\n",string[0],string[2],string[3]);
	printf(defines.allocator,"#define COMPABILITY_NO_PRECISION 1\n");
	printf(defines.allocator,"#define COMPABILITY_MODERN_GLSL 1\n");
#endif
	
	auto versionHeaderLength = defines.allocator.size();
	if(src) allocator->deallocate(src);

	auto srcLen = versionHeaderLength+source.length();
	src = (uint8*)allocator->allocate(srcLen+1);
	memcpy(src,defines.allocator.bufferBase(),versionHeaderLength);
	memcpy(src+versionHeaderLength,source.begin,source.length());
	src[srcLen] = '\0';

	auto logger = services::logging();
	if(logger->priority() <= application::logging::Trace){
		logger->trace("Final GLSL shader:");
		//logger->trace((char*)src);
	}

	return core::Bytes(src,srcLen);
#else

	return source;
#endif
}

} }
