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
	printf(defines.allocator,"#define lowp\n");
	printf(defines.allocator,"#define mediump\n");
	printf(defines.allocator,"#define highp\n");
	printf(defines.allocator,"#define ATTRIBUTE_IN in\n");
	printf(defines.allocator,"#define VARYING_IN in\n");
	printf(defines.allocator,"#define VARYING_OUT out\n");
	printf(defines.allocator,"#define BONES_LENGTH 64\n");
	printf(defines.allocator,"#define BONES_PER_VERTEX 4\n");
	printf(defines.allocator,"#define DEF_LIGHT_STRUCT struct Light { vec4 position; vec4 diffuse; vec4 specular; vec4 parameters; };\n");
	printf(defines.allocator,"#define DEF_MAT_STRUCT   struct Material { vec4 parameters; }\n");
	printf(defines.allocator,"#define DEF_PIXEL_SHADER_OUT out vec4 arpheg_ps_out\n");
	printf(defines.allocator,"#define PIXEL_SHADER_OUT arpheg_ps_out\n");
#else
	printf(defines.allocator,"#define ATTRIBUTE_IN attibute\n");
	printf(defines.allocator,"#define VARYING_IN varying\n");
	printf(defines.allocator,"#define VARYING_OUT varying\n");
	printf(defines.allocator,"#define DEF_PIXEL_SHADER_OUT");
	printf(defines.allocator,"#define PIXEL_SHADER_OUT gl_FragColor");
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
