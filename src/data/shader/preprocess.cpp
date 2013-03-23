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
