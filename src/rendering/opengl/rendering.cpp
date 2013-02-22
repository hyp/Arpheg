#include <assert.h>
#include <string.h>

#include "../../core/allocatorNew.h"
#include "../../services.h"
#include "gl.h"
#include "rendering.h"

#ifdef ARPHEG_RENDERING_GLES
	#define GL_NOVAO
#endif

namespace rendering {
	Service::Service() : objectEmulationBuffer(2048,core::memory::globalAllocator()), samplerEmulationBuffer(2048,core::memory::globalAllocator()) {
		//Set the initial state.
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glEnable (GL_DEPTH_TEST);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		bind(topology::Triangle);

		//NB emulation: Create a default sampler object
		texture::SamplerDescriptor defaultSampler;
		defaultSampler.filter = texture::SamplerDescriptor::MagPoint|texture::SamplerDescriptor::MinPoint;
		defaultSampler.adressModes[0] = texture::SamplerDescriptor::Clamp;
		defaultSampler.adressModes[1] = texture::SamplerDescriptor::Clamp;
		defaultSampler.adressModes[2] = texture::SamplerDescriptor::Clamp;
		defaultSampler.maxAnisotropy = 0;defaultSampler.mipLodBias = 0.0f;
		samplerEmulationDefault = create(defaultSampler).id;
		for(uint32 i = 0;i < kMaxTextureSlots;i++) samplerEmulationSlots[i] = samplerEmulationDefault;
	}
	Service::~Service() {
	}

	static inline GLenum bufferType(Buffer::Type type) {
		return type == Buffer::Vertex? GL_ARRAY_BUFFER : type == Buffer::Index? GL_ELEMENT_ARRAY_BUFFER : GL_UNIFORM_BUFFER;
	}
	Buffer Service::create (Buffer::Type type,bool dynamic,size_t size,void* data) {
		Buffer buffer;
		CHECK_GL(glGenBuffers(1, &buffer.id));
		assert(buffer.id!=0);
		GLenum target = bufferType(type);
		CHECK_GL(glBindBuffer(target, buffer.id));
		CHECK_GL(glBufferData(target, size, data, dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
		return buffer;
	}
	void   Service::update (Buffer::Type type,Buffer buffer,size_t offset,void* data,size_t size) {
		GLenum target = bufferType(type);
		CHECK_GL(glBindBuffer(target, buffer.id));
		CHECK_GL(glBufferSubData(target, offset,size,data));
	}
	void   Service::release(Buffer buffer) {
		if(buffer.id){
			CHECK_GL(glDeleteBuffers(1,&buffer.id));
		}
	}

	static void onGLShaderError(GLuint id, bool shader = true) {
		char buf[256];
		if (!shader) glGetProgramInfoLog(id, 256, 0, buf);
		else glGetShaderInfoLog(id, 256, 0, buf);
		services::logging()->resourceError("GLSL Shader/Program compilation failure!",buf);
	}
	static GLuint createShader(GLenum type, const char* src,size_t length) {
		GLuint id = glCreateShader(type);
		if(!length) length = strlen(src);
		const GLint len = (GLint) length;
		CHECK_GL(glShaderSource(id, 1, &src, &len));
		CHECK_GL(glCompileShader(id));
		//check
		GLint result = GL_FALSE;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);
		if (!result) onGLShaderError(id);
		return id;
	}
	Shader Service::create(Shader::Type type,const char* src,size_t sourceLength) {
		Shader shader = { createShader(type == Shader::Vertex? GL_VERTEX_SHADER : type == Shader::Pixel? GL_FRAGMENT_SHADER : GL_GEOMETRY_SHADER,src,sourceLength) };
		return shader;
	}
	void Service::release(Shader shader) {
		assert(shader.id!=0);
		CHECK_GL(glDeleteShader(shader.id));
	}

	static inline GLenum typeType(uint32 id){
		//TODO full
		if(id == CoreTypeDescriptor::TFloat) return GL_FLOAT;
		else if(id == CoreTypeDescriptor::TInt32) return GL_INT;
		else if(id == CoreTypeDescriptor::TInt8)  return GL_BYTE;
		else if(id == CoreTypeDescriptor::TUint8) return GL_UNSIGNED_BYTE;
		else if(id == CoreTypeDescriptor::TUint16) return GL_UNSIGNED_SHORT;
		else if(id == CoreTypeDescriptor::THalf) return GL_HALF_FLOAT;
		return 0;
	}
	static uint32 bindVertexInput(CoreTypeDescriptor* fields, uint32 count,void* offset = nullptr){
		uint32 i = 0;
		GLsizei stride = 0;
		for(auto field = fields,end = fields + count;field < end;++field)
			stride += field->size();
		for(auto field = fields,end = fields + count;field < end;++field,++i){
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, (GLint)field->count, typeType(field->id), field->id == CoreTypeDescriptor::TUint8? GL_TRUE: GL_FALSE, stride, offset);
			offset = (GLvoid*) ( ((uint8*)offset) + field->size());
		}
		return i;
	}
	Pipeline::Constant::Constant(const char* name){
		this->location = -1;
		this->shaderType = 0;
		this->coreTypeId = 0;
		this->count = count;
		this->name = name;
	}
	Pipeline Service::create(VertexDescriptor input,Shader vertexShader,Shader pixelShader) {
		GLuint program = glCreateProgram();

		CHECK_GL(glAttachShader(program, vertexShader.id));
		CHECK_GL(glAttachShader(program, pixelShader.id));

		for(uint32 i = 0 ,end = input.count;i < end;++i)
			CHECK_GL(glBindAttribLocation(program,GLuint(i),input.slots[i]));
		CHECK_GL(glLinkProgram(program));
		//check
		GLint result = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		if (!result) onGLShaderError(program, false);

		GLuint vao;
#ifndef GL_NOVAO
		//vao
		glGenVertexArrays(1, &vao);	
		glBindVertexArray(vao);
		bindVertexInput(input.fields,input.count);
		//glBindVertexArray(0);
#endif

		inputDescriptorBound = false;

		Pipeline pipeline = { program,input.count,input.fields };
		return pipeline;
	}
	Pipeline Service::create(VertexDescriptor input,Shader shaders[],size_t count) {
		GLuint program = glCreateProgram();
		for(size_t i =0;i<count;++i)
			CHECK_GL(glAttachShader(program, shaders[i].id));
		for(uint32 i = 0 ,end = input.count;i < end;++i)
			CHECK_GL(glBindAttribLocation(program,GLuint(i),input.slots[i]));
		CHECK_GL(glLinkProgram(program));
		//check
		GLint result = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		if (!result) onGLShaderError(program, false);

		GLuint vao;
#ifndef GL_NOVAO
		//vao
		glGenVertexArrays(1, &vao);	
		glBindVertexArray(vao);
		//bindVertexInput(input.fields,input.count);
		//glBindVertexArray(0);
#endif

		inputDescriptorBound = false;

		Pipeline pipeline = { program,input.count,input.fields };
		return pipeline;
	}
	void Service::release(Pipeline pipeline) {
		glDeleteProgram(pipeline.id);
	}

	void Service::clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void Service::clear(const vec4f& color) {
		glClearColor(color.x,color.y,color.z,color.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void Service::bind(Pipeline pipeline) {
		currentPipeline = pipeline.id;
		glUseProgram(pipeline.id);

		inputDescriptorBound = false;
		inputDescriptor      = pipeline.inputDescriptor;
		inputDescriptorCount = pipeline.inputDescriptorCount;
	}

	enum {
		TFloat,TFloat2,TFloat3,TFloat4,
		TInt,TInt2,TInt3,TInt4,
		TMat44,TMat33,TMat22,
	};
	static uint8 uniformType(GLenum type){
		switch(type){
		case GL_FLOAT:      return TFloat;
		case GL_FLOAT_VEC2: return TFloat2;
		case GL_FLOAT_VEC3: return TFloat3;
		case GL_FLOAT_VEC4: return TFloat4;
		case GL_FLOAT_MAT4: return TMat44;
			
		case GL_INT: case GL_SAMPLER_2D:  case GL_SAMPLER_3D: case GL_SAMPLER_1D: case GL_SAMPLER_CUBE: case GL_BOOL: 
		case GL_SAMPLER_1D_ARRAY:
		case GL_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_1D_ARRAY_SHADOW:
		case GL_SAMPLER_2D_ARRAY_SHADOW:
		case GL_SAMPLER_CUBE_SHADOW:
		case GL_INT_SAMPLER_1D:
		case GL_INT_SAMPLER_2D:
		case GL_INT_SAMPLER_3D:
		case GL_INT_SAMPLER_CUBE:
		case GL_INT_SAMPLER_1D_ARRAY:
		case GL_INT_SAMPLER_2D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_1D:
		case GL_UNSIGNED_INT_SAMPLER_2D:
		case GL_UNSIGNED_INT_SAMPLER_3D:
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
		case GL_SAMPLER_BUFFER: case GL_INT_SAMPLER_BUFFER: case GL_UNSIGNED_INT_SAMPLER_BUFFER:
			return TInt;
		case GL_INT_VEC2: case GL_BOOL_VEC2: return TInt2;
		case GL_INT_VEC3: case GL_BOOL_VEC3: return TInt3;
		case GL_INT_VEC4: case GL_BOOL_VEC4: return TInt4;

		case GL_FLOAT_MAT3: return TMat33;
		case GL_FLOAT_MAT2: return TMat22;
		}
		assert(false && "Invalid control flow!");
		return 0;
	}
	static void initUniform(uint32 pipeline,Pipeline::Constant& constant){
		char buffer[256];
		GLint count = 0, length;
		glGetProgramiv(pipeline,GL_ACTIVE_UNIFORMS,&count);
		glGetProgramiv(pipeline,GL_ACTIVE_UNIFORM_MAX_LENGTH,&length);
		assert(length < 256);
		GLsizei len,size;
		GLenum  type;
		for(int32 i = 0;i < count;++i){
			glGetActiveUniform(pipeline,i,sizeof(buffer),&len,&size,&type,buffer);
			if(!strcmp(constant.name,buffer)) {
				constant.coreTypeId = uniformType(type);
				constant.count = size;
				constant.location   = glGetUniformLocation(pipeline,constant.name);
				return;
			}
		}
		//assert(false);
	}
	void Service::bind(Pipeline::Constant& constant,void* data) {
		if(constant.location == -1){
			initUniform(currentPipeline,constant);
			if(constant.location == -1) return;
		}
		GLint location = constant.location;

		switch(constant.coreTypeId){
		case TFloat:  glUniform1fv(location,constant.count,(const GLfloat*)data); break;
		case TFloat2: glUniform2fv(location,constant.count,(const GLfloat*)data); break;
		case TFloat3: glUniform3fv(location,constant.count,(const GLfloat*)data); break;
		case TFloat4: glUniform4fv(location,constant.count,(const GLfloat*)data); break;
		case TInt:    glUniform1iv(location,constant.count,(const GLint*)data); break;
		case TInt2:   glUniform2iv(location,constant.count,(const GLint*)data); break;
		case TInt3:   glUniform3iv(location,constant.count,(const GLint*)data); break;
		case TInt4:   glUniform4iv(location,constant.count,(const GLint*)data); break;
		case TMat44:  glUniformMatrix4fv(location,constant.count,false,(const GLfloat*)data); break;
		case TMat33:  glUniformMatrix3fv(location,constant.count,false,(const GLfloat*)data); break;
		case TMat22:  glUniformMatrix2fv(location,constant.count,false,(const GLfloat*)data); break;
		}
	}
	void Service::bind(Pipeline::Constant& constant,const mat44f& matrix) {
		if(constant.location == -1){
			initUniform(currentPipeline,constant);
			if(constant.location == -1) return;
		}
		GLint location = constant.location;
		assert(constant.coreTypeId == TMat44);
		glUniformMatrix4fv(location,constant.count,false,(const GLfloat*)&matrix.a.x);
	}
	void Service::bind(Pipeline::Constant& constant,Buffer data,size_t offset,size_t size) {
		/*if(constant.location == -1){
			constant.location = glGetUniformBlockIndex(currentPipeline,constant.name);
			if(constant.location == -1) return;
		}
		if(offset == 0 && size == 0)  glBindBufferBase     (GL_UNIFORM_BUFFER,0,data.id);
		else glBindBufferRange(GL_UNIFORM_BUFFER,0,data.id,offset,size);
		glUniformBlockBinding(currentPipeline,constant.location,0);*/
	}

	void Service::bindVertices(Buffer buffer) {
#ifndef PLATFORM_RENDERING_BUFFERS_ONLY
		if(inputVertices){
			inputVertices = nullptr;
			inputIndices = nullptr;
		}
#endif
		CHECK_GL(glBindBuffer(GL_ARRAY_BUFFER,buffer.id));
		//Vertex attributes will have to be rebound - this might not be necessary.
		inputDescriptorBound = false;
	}
	void Service::bindIndices (Buffer buffer,uint32 indexSize) {
#ifdef PLATFORM_RENDERING_GLES
		//NB: no uint indices in gles 2 unless OES_element_index_uint is present
		assert(indexSize < 3);
#endif
		assert(indexSize == 1 || indexSize == 2 || indexSize == 4);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer.id);
		currentIndexSize = indexSize == 2? GL_UNSIGNED_SHORT : (indexSize == 1? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT);
	}
#ifndef PLATFORM_RENDERING_BUFFERS_ONLY
	void Service::bindVerticesIndicesStream(void* vertices,void* indices) {
		if(!inputVertices){
			//Make sure no buffers are bound
			glBindBuffer(GL_ARRAY_BUFFER,0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
		}
		inputVertices = vertices;
		inputIndices = indices;
		inputDescriptorBound = false;
	}
#endif

	static inline GLenum primitiveMode(topology::Primitive type) {
		using namespace topology;
		switch(type){
			case Point: return GL_POINTS;
			case Line: return GL_LINES;
			case LineLoop: return GL_LINE_LOOP;
			case LineStrip: return GL_LINE_STRIP;
			case Triangle: return GL_TRIANGLES;
			case TriangleStrip: return GL_TRIANGLE_STRIP;
			case TriangleFan: return GL_TRIANGLE_FAN;
		}
		assert(false && "Invalid control flow!");
		return 0;
	}
	void Service::bind(topology::Primitive primitiveTopology) {
		currentPrimitiveTopology = primitiveMode(primitiveTopology);
	}
	void Service::draw(uint32 offset,uint32 count) {
//#ifdef GL_NOVAO
		if(!inputDescriptorBound){
			auto oldEnabled = inputDescriptorEnabled; 
			inputDescriptorEnabled = bindVertexInput(inputDescriptor,inputDescriptorCount,inputVertices);
			//Disable the previous attributes.
			for(auto i = inputDescriptorEnabled; i < oldEnabled; ++i) glDisableVertexAttribArray(i);
			inputDescriptorBound = true;
		}
//#endif
		glDrawArrays(currentPrimitiveTopology,offset,count);
	}
	void Service::drawIndexed(uint32 offset,uint32 count) {
//#ifdef GL_NOVAO
		if(!inputDescriptorBound){
			auto oldEnabled = inputDescriptorEnabled; 
			inputDescriptorEnabled = bindVertexInput(inputDescriptor,inputDescriptorCount,inputVertices);
			//Disable the previous attributes.
			for(auto i = inputDescriptorEnabled; i < oldEnabled; ++i) glDisableVertexAttribArray(i);
			inputDescriptorBound = true;
		}
//#endif
#ifdef PLATFORM_RENDERING_GLES
		//NB: no drawRangeElements in gles 2
		assert(offset == 0);
#endif
#ifdef PLATFORM_RENDERING_BUFFERS_ONLY
		const void* inputIndices = nullptr;
#endif
		glDrawElements(currentPrimitiveTopology,count,currentIndexSize,inputIndices);
	}
	static inline GLenum blendMode(blending::Blend mode){
		using namespace blending;
		switch(mode) {
		case Zero: return GL_ZERO;
		case One:  return GL_ONE;
		case SrcColor: return GL_SRC_COLOR;
		case InvertedSrcColor: return GL_ONE_MINUS_SRC_COLOR;
		case SrcAlpha: return GL_SRC_ALPHA;
		case InvertedSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
		case DestAlpha: return GL_DST_ALPHA;
		case InvertedDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
		case DestColor: return GL_DST_COLOR;
		case InvertedDestColor: return GL_ONE_MINUS_DST_COLOR;
		}
		assert(false && "Invalid control flow!");
		return 0;
	}
	void Service::bind(const blending::State& state) {
		if(state.enabled != blendingState.enabled){
			if(state.enabled) glEnable(GL_BLEND);
			else {
				glDisable(GL_BLEND);
				blendingState.enabled = false;
				return;
			}
		}
		if(state.srcRgb == state.srcAlpha && state.destRgb == state.destAlpha){
			glBlendFunc(blendMode(state.srcRgb),blendMode(state.destRgb));
		} else {
			glBlendFuncSeparate(blendMode(state.srcRgb),blendMode(state.destRgb),blendMode(state.srcAlpha),blendMode(state.destAlpha));
		}
		blendingState = state;
		//TODO
	}
	void Service::bind(const rasterization::State& state) {
#ifdef PLATFORM_RENDERING_GLES
		assert(state.fillMode == rasterization::Solid);//Lame
#endif
		if(state.cullMode != rasterState.cullMode){
			if(state.cullMode == rasterization::CullNone) glDisable(GL_CULL_FACE);
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(state.cullMode == rasterization::CullBack? GL_BACK : GL_FRONT);
			}
		}
		rasterState = state;
	}
	void Service::bind(const Viewport& viewport) {
		glViewport(viewport.position.x,viewport.position.y,viewport.size.x,viewport.size.y);
	}

	void Service::servicePreStep() {
#ifndef PLATFORM_RENDERING_BUFFERS_ONLY
		inputVertices = nullptr;
		inputIndices = nullptr;
#endif
		inputDescriptorBound = false;
		inputDescriptorEnabled = 0;
		currentPipeline = 0;
	}
}