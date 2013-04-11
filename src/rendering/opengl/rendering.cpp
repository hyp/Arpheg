#include <string.h>

#include "../../core/allocatorNew.h"
#include "../../core/assert.h"
#include "../../services.h"
#include "gl.h"
#include "rendering.h"

namespace rendering {

#ifndef ARPHEG_RENDERING_GL_VAO
	struct EmulatedVAO {
		GLsizei stride;
		uint32 count;
		struct Field {
			uint8 size;
			uint8 type;
			uint8 normalized;
			uint8 totalSize;
		};
	};
#endif



	Service::Service() : 
#ifndef ARPHEG_RENDERING_GL_VAO
		vaoEmulationBuffer(2048,nullptr,core::BufferGrowOnOverflow),
#endif
		objectEmulationBuffer(2048,nullptr),
		samplerEmulationBuffer(2048,nullptr) {
		
		//Set the initial state.
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glEnable (GL_DEPTH_TEST);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);

		blendingState_.enabled = false;
		rasterState_.cullMode = rasterization::CullNone;
		rasterState_.fillMode = rasterization::Solid;
		rasterState_.isFrontFaceCounterClockwise = false;

		//NB emulation: Create a default sampler object
		texture::SamplerDescriptor defaultSampler;
		defaultSampler.filter = texture::SamplerDescriptor::MagPoint|texture::SamplerDescriptor::MinPoint;
		defaultSampler.adressModes[0] = texture::SamplerDescriptor::Clamp;
		defaultSampler.adressModes[1] = texture::SamplerDescriptor::Clamp;
		defaultSampler.adressModes[2] = texture::SamplerDescriptor::Clamp;
		defaultSampler.maxAnisotropy = 0;defaultSampler.mipLodBias = 0.0f;
		samplerEmulationDefault = create(defaultSampler).id;
		for(uint32 i = 0;i < kMaxTextureSlots;i++) samplerEmulationSlots[i] = samplerEmulationDefault;

		hasDsa_ = 0;
#ifndef ARPHEG_RENDERING_GLES
		if(context()->extensionSupported(opengl::extensions::EXT_direct_state_access)){
			hasDsa_ = 1;
		}
#endif
	}
	Service::~Service() {
	}

	static inline GLenum bufferType(Buffer::Type type) {
		return type == Buffer::Vertex? GL_ARRAY_BUFFER : type == Buffer::Index? GL_ELEMENT_ARRAY_BUFFER : GL_UNIFORM_BUFFER;
	}
	Buffer Service::create (Buffer::Type type,bool dynamic,size_t size,void* data) {
#ifdef ARPHEG_RENDERING_GL_VAO
		if(type == Buffer::Index) glBindVertexArray(0); //NB: important
#endif
		Buffer buffer;
		CHECK_GL(glGenBuffers(1, &buffer.id));
		assert(buffer.id!=0);
		GLenum target = bufferType(type);
		CHECK_GL(glBindBuffer(target, buffer.id));
		CHECK_GL(glBufferData(target, size, data, dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
		return buffer;
	}
	void Service::recreate (Buffer::Type type,Buffer buffer,bool dynamic,size_t size,void* data) {
#ifdef ARPHEG_RENDERING_GL_VAO
		if(type == Buffer::Index) glBindVertexArray(0); //NB: important
#endif
		GLenum target = bufferType(type);
		glBindBuffer(target, buffer.id);
		glBufferData(target, size, data, dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	}
#ifndef ARPHEG_RENDERING_GLES
	Buffer::Mapping   Service::map(Buffer::Type type,Buffer buffer){
		GLenum target = bufferType(type);
		Buffer::Mapping result;
		result.id = buffer.id;
		result.type = type;
#ifdef ARPHEG_RENDERING_GL_VAO
		if(type == Buffer::Index) glBindVertexArray(0); //NB: important
#endif
		glBindBuffer(target, buffer.id);
		result.data = glMapBuffer(target,GL_WRITE_ONLY);
		return result;
	}
	void   Service::unmap(const Buffer::Mapping& mapping) {
		GLenum target = bufferType(mapping.type);
		glBindBuffer(target, mapping.id);
		glUnmapBuffer(target);
	}
#endif
	void   Service::update (Buffer::Type type,Buffer buffer,size_t offset,void* data,size_t size) {
#ifdef ARPHEG_RENDERING_GL_VAO
		if(type == Buffer::Index) glBindVertexArray(0); //NB: important
#endif
		GLenum target = bufferType(type);
		CHECK_GL(glBindBuffer(target, buffer.id));
		CHECK_GL(glBufferSubData(target, offset,size,data));
	}
	void   Service::release(Buffer buffer) {
		if(buffer.id){
			CHECK_GL(glDeleteBuffers(1,&buffer.id));
		}
	}
	static inline GLenum typeType(uint32 id){
		id &= (~core::TypeDescriptor::kNormalized);
		if(id == core::TypeDescriptor::TFloat) return GL_FLOAT;
		else if(id == core::TypeDescriptor::TInt32) return GL_INT;
		else if(id == core::TypeDescriptor::TUint32) return GL_UNSIGNED_INT;
		else if(id == core::TypeDescriptor::TInt8)  return GL_BYTE;
		else if(id == core::TypeDescriptor::TUint8) return GL_UNSIGNED_BYTE;
		else if(id == core::TypeDescriptor::TInt16) return GL_SHORT;
		else if(id == core::TypeDescriptor::TUint16) return GL_UNSIGNED_SHORT;
		else if(id == core::TypeDescriptor::THalf) return GL_HALF_FLOAT;
		else if(id == core::TypeDescriptor::TDouble) return GL_DOUBLE;
#ifndef ARPHEG_RENDERING_GLES
		else if(id == core::TypeDescriptor::TInt3x10_2) return GL_INT_2_10_10_10_REV;
		else if(id == core::TypeDescriptor::TUint3x10_2) return GL_UNSIGNED_INT_2_10_10_10_REV;
#endif
		assert(false && "Unsupported vertex attribute type");
		return 0;
	}

#ifndef ARPHEG_RENDERING_GL_VAO
	//TODO fix
	static uint32 emulatedVAOnew(core::BufferAllocator& buffer,core::TypeDescriptor* fields, uint32 count){
		uint32 i = 0;
		GLsizei stride = 0;
		for(auto field = fields,end = fields + count;field < end;++field)
			stride += field->size();
		auto vao = (EmulatedVAO*)buffer.allocate(sizeof(EmulatedVAO) + sizeof(EmulatedVAO::Field)*count);
		vao->stride = stride;vao->count = count;
		auto dest = (EmulatedVAO::Field*)(vao+1);

		for(auto field = fields,end = fields + count;field < end;++field,++i,++dest){
			dest->size = field->count;
			uint32 glType = typeType(field->id);
			assert((glType - GL_BYTE) < 255);
			dest->type = uint8(glType - GL_BYTE);
			dest->normalized = (field->id & core::TypeDescriptor::kNormalized) != 0;
			auto sz = field->size();
			assert(sz < 255);
			dest->totalSize = uint8(sz);
		}
		return buffer.toOffset(vao);
	}
	static uint32 emulatedVAOgen(core::BufferAllocator& buffer,core::TypeDescriptor* fields, uint32 count){
		//Check if such vao already exists.
		return emulatedVAOnew(buffer,fields,count);
	}
	static uint32 emulatedVAObind(core::BufferAllocator& buffer,uint32 id,uint32 oldAttribEnabledCount){

		auto vao = (EmulatedVAO*)(buffer.toPointer(id));
		GLsizei stride = vao->stride; auto count = vao->count;
		//NB: disable the previous vertex attributes
		for(auto i = count; i < oldAttribEnabledCount; ++i) glDisableVertexAttribArray(i);
		
		auto fields = (EmulatedVAO::Field*)(vao+1);
		const void* offset = (const void*)0;
		for(uint32 i = 0;i<count;++i){
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i,GLint(fields->size),GLenum(fields->type),GLboolean(fields->normalized),stride,offset);
			offset  = (const void*) ( ((const uint8*)offset) + size_t(fields->totalSize));
		}
		
		return count;
	}
#endif
	static uint32 bindVertexInput(core::TypeDescriptor* fields, uint32 count,const void* offset = nullptr){
		uint32 i = 0;
		GLsizei stride = 0;
		for(auto field = fields,end = fields + count;field < end;++field)
			stride += field->size();
		for(auto field = fields,end = fields + count;field < end;++field,++i){
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, (GLint)field->count, typeType(field->id),(field->id & core::TypeDescriptor::kNormalized)!=0, stride, offset);
			offset = (const void*) ( ((const uint8*)offset) + field->size());
		}
		return i;
	}
	Mesh   Service::create(Buffer vertices,Buffer indices,const VertexDescriptor& vertexLayout){
#ifdef ARPHEG_RENDERING_GL_VAO
		Mesh mesh = {vertices,indices,0};
		CHECK_GL(glGenVertexArrays(1, &mesh.vao));	
		CHECK_GL(glBindVertexArray(mesh.vao));
		glBindBuffer(GL_ARRAY_BUFFER,vertices.id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices.id);
		bindVertexInput(vertexLayout.fields,vertexLayout.count);
		glBindVertexArray(0);
#else
		Mesh mesh = {vertices,indices,emulatedVAOgen(vaoEmulationBuffer,vertexLayout.fields,vertexLayout.count)};
#endif
		return mesh;
	}
	void Service::update(Mesh& mesh,Buffer vertices,Buffer indices,const VertexDescriptor& vertexLayout) {
#ifdef ARPHEG_RENDERING_GL_VAO
		mesh.vbo = vertices;
		mesh.ibo = indices;
		CHECK_GL(glBindVertexArray(mesh.vao));
		glBindBuffer(GL_ARRAY_BUFFER,vertices.id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices.id);
		bindVertexInput(vertexLayout.fields,vertexLayout.count);
		glBindVertexArray(0);
#else
		mesh.vertices = vertices;
		mesh.indices  = indices;
		mesh.vao = emulatedVAOgen(vaoEmulationBuffer,vertexLayout.fields,vertexLayout.count);
#endif
	}
	void   Service::release(const Mesh& mesh){
#ifdef ARPHEG_RENDERING_GL_VAO
		assert(mesh.vao);
		glDeleteVertexArrays(1,&mesh.vao);
#endif
	}

	static void onGLShaderError(GLuint id, bool shader = true) {
		char buf[512];
		if (!shader) glGetProgramInfoLog(id, sizeof(buf), 0, buf);
		else glGetShaderInfoLog(id, sizeof(buf), 0, buf);
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
		GLenum t;
		switch(type){
		case Shader::Vertex: t = GL_VERTEX_SHADER; break;
		case Shader::Pixel:  t = GL_FRAGMENT_SHADER; break;
#ifndef ARPHEG_RENDERING_GLES
		//TODO api support
		case Shader::Geometry: t = GL_GEOMETRY_SHADER; break;
		case Shader::TesselationControl: t = GL_TESS_CONTROL_SHADER; break;
		case Shader::TesselationEvaluation: t = GL_TESS_EVALUATION_SHADER; break;
#endif
		default:
			assert(false && "Invalid shader type");
		}
		Shader shader = { createShader(t,src,sourceLength) };
		return shader;
	}
	void Service::release(Shader shader) {
		assert(shader.id!=0);
		CHECK_GL(glDeleteShader(shader.id));
	}

	Pipeline::Constant::Constant(const char* name){
		this->location = -1;
		this->shaderType = 0;
		this->coreTypeId = 0;
		this->count = count;
		this->name = name;
	}

	Pipeline::GLSLLayout::GLSLLayout() {
		vertexAttributes = nullptr;
		vertexAttributeCount = 0;
		pixelAttributes = nullptr;
		pixelAttributeCount = 0;
	}
	Pipeline Service::create(Shader shaders[],size_t count,const Pipeline::GLSLLayout& glslLayout) {
		GLuint program = glCreateProgram();
		for(size_t i =0;i<count;++i)
			CHECK_GL(glAttachShader(program, shaders[i].id));
		for(uint32 i = 0 ,end = glslLayout.vertexAttributeCount;i < end;++i)
			CHECK_GL(glBindAttribLocation(program,GLuint(i),glslLayout.vertexAttributes[i]));
#ifndef ARPHEG_RENDERING_GLES
		for(uint32 i = 0, end = glslLayout.pixelAttributeCount;i<end;++i)
			CHECK_GL(glBindFragDataLocation(program,GLuint(i),glslLayout.pixelAttributes[i]));
#endif
		CHECK_GL(glLinkProgram(program));
		//check
		GLint result = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		if (!result) onGLShaderError(program, false);

		Pipeline pipeline = { program };
		return pipeline;
	}
	void Service::release(Pipeline pipeline) {
		glDeleteProgram(pipeline.id);
	}

	void Service::clear(const vec4f& color,bool clearColour,bool clearDepth,bool clearStencil) {
		GLbitfield mask = (clearColour? GL_COLOR_BUFFER_BIT : 0) | (clearDepth? GL_DEPTH_BUFFER_BIT : 0) | (clearStencil? GL_STENCIL_BUFFER_BIT : 0);
		if(clearColour) glClearColor(color.x,color.y,color.z,color.w);
		glClear(mask);
	}
	void Service::bind(Pipeline pipeline) {
		currentPipeline = pipeline.id;
		glUseProgram(pipeline.id);
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
		constant.location = -2;
	}
	void Service::bind(Pipeline::Constant& constant,void* data) {
		if(constant.location < 0){
			if(constant.location == -2) return;
			initUniform(currentPipeline,constant);
			if(constant.location == -2) return;
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
		if(constant.location < 0){
			if(constant.location == -2) return;
			initUniform(currentPipeline,constant);
			if(constant.location == -2) return;
		}
		GLint location = constant.location;
		assert(constant.coreTypeId == TMat44);
		glUniformMatrix4fv(location,constant.count,false,(const GLfloat*)&matrix.a.x);
	}
	void Service::bind(Pipeline::Constant& constant,const mat44f* matrix) {
		if(constant.location < 0){
			if(constant.location == -2) return;
			initUniform(currentPipeline,constant);
			if(constant.location == -2) return;
		}

		GLint location = constant.location;
		assert(constant.coreTypeId == TMat44);
		glUniformMatrix4fv(location,constant.count,false,(const GLfloat*)&matrix->a.x);
	}
	void Service::bindConstantSlot(Pipeline pipeline,const Pipeline::Constant& constant,uint32 slot){
		//At least this is DSA! Thanks ARB..
		auto loc = glGetUniformBlockIndex(pipeline.id,constant.name);
		if(loc == GL_INVALID_INDEX) return;
		glUniformBlockBinding(pipeline.id,loc,slot);
	}
	void Service::bindConstantBuffer(Buffer data,uint32 slot,uint32 offset,uint32 size){
		if(offset == 0 && size == 0)  glBindBufferBase(GL_UNIFORM_BUFFER,slot,data.id);
		else glBindBufferRange(GL_UNIFORM_BUFFER,slot,data.id,offset,size);
	}

	void Service::bind(Pipeline::Constant& constant,Buffer data,size_t offset,size_t size) {
		if(constant.location < 0){
			if(constant.location == -2) return;
			constant.location = glGetUniformBlockIndex(currentPipeline,constant.name);
			if(constant.location == -2) return;
		}

		uint32 slot = 0;
		if(offset == 0 && size == 0)  glBindBufferBase(GL_UNIFORM_BUFFER,slot,data.id);
		else glBindBufferRange(GL_UNIFORM_BUFFER,slot,data.id,offset,size);
		glUniformBlockBinding(currentPipeline,constant.location,slot);
	}

	/*void Service::bindVertices(Buffer buffer) {
		glBindBuffer(GL_ARRAY_BUFFER,buffer.id);
	}
	void Service::bindIndices (Buffer buffer,uint32 indexSize) {
#ifdef ARPHEG_RENDERING_GLES
		//NB: no uint indices in gles 2 unless OES_element_index_uint is present
		assert(indexSize < 3);
#endif
		assert(indexSize == 1 || indexSize == 2 || indexSize == 4);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer.id);
		currentIndexSize = indexSize == 2? GL_UNSIGNED_SHORT : (indexSize == 1? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT);
	}*/

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
			//case Patch: return GL_PATCHES​;
		}
		assert(false && "Invalid control flow!");
		return 0;
	}
	void Service::bind(const Mesh& mesh,topology::Primitive primitiveTopology,uint32 indexSize) {
#ifdef ARPHEG_RENDERING_GLES
		//NB: no uint indices in gles 2 unless OES_element_index_uint is present
		assert(indexSize < 3);
#endif
		assert(indexSize == 0 || indexSize == 1 || indexSize == 2 || indexSize == 4);
		static_assert(GL_UNSIGNED_SHORT <= 0xFFFF &&
			GL_UNSIGNED_BYTE <= 0xFFFF &&
			GL_UNSIGNED_INT <= 0xFFFF,"rendering::Service currentIndexSize is too small!");

#ifdef ARPHEG_RENDERING_GL_VAO
		glBindVertexArray(mesh.vao);
		currentIndexSize = indexSize == 2? GL_UNSIGNED_SHORT : (indexSize == 1? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT);
#else
		glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo.id);
		if(indexSize) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh.ibo.id);
		//if(currentVao != mesh.vao){
			vaoEmulation_oldAttribEnabledCount = emulatedVAObind(mesh.vao,vaoEmulation_oldAttribEnabledCount);
			//currentVao = mesh.vao;
		//}
		currentIndexSize = indexSize == 2? GL_UNSIGNED_SHORT : (indexSize == 1? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT);
#endif
		currentPrimitiveTopology = primitiveTopology == topology::Triangle? GL_TRIANGLES: (primitiveMode(primitiveTopology));
	}
	void Service::draw(uint32 offset,uint32 count) {
		glDrawArrays(currentPrimitiveTopology,offset,count);
	}
	void Service::drawIndexed(uint32 offset,uint32 count) {
#ifdef ARPHEG_RENDERING_GLES
		//NB: no drawRangeElements in gles 2
		assert(offset == 0);
		glDrawElements(currentPrimitiveTopology,count,GLenum(currentIndexSize),nullptr);
#else
		glDrawElements(currentPrimitiveTopology,count,GLenum(currentIndexSize),(const void*)offset);
#endif
	}
	void Service::drawIndexed(uint32 offset,uint32 count,uint32 baseVertex) {
#ifdef ARPHEG_RENDERING_GLES
		assert(false && "Unsupported!");
#else
		glDrawElementsBaseVertex(currentPrimitiveTopology,count,GLenum(currentIndexSize),(const void*)offset,baseVertex);
#endif
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
		if(state.enabled != blendingState_.enabled){
			if(state.enabled) glEnable(GL_BLEND);
			else {
				glDisable(GL_BLEND);
				blendingState_.enabled = false;
				return;
			}
		} else return;
		if(state.srcRgb == state.srcAlpha && state.destRgb == state.destAlpha){
			glBlendFunc(blendMode(state.srcRgb),blendMode(state.destRgb));
		} else {
			glBlendFuncSeparate(blendMode(state.srcRgb),blendMode(state.destRgb),blendMode(state.srcAlpha),blendMode(state.destAlpha));
		}
		blendingState_ = state;
	}
	void Service::bind(const rasterization::State& state) {
		if(state.cullMode != rasterState_.cullMode){
			if(state.cullMode == rasterization::CullNone) glDisable(GL_CULL_FACE);
			else {
				glEnable(GL_CULL_FACE);
				glCullFace(state.cullMode == rasterization::CullBack? GL_BACK : GL_FRONT);
			}
		}
#ifndef ARPHEG_RENDERING_GLES
		if(state.fillMode != rasterState_.fillMode){
			glPolygonMode(GL_FRONT_AND_BACK,state.fillMode == rasterization::Solid? GL_FILL : GL_LINES);
		}
#else
		assert(state.fillMode == rasterization::Solid);//Lame
#endif
		rasterState_ = state;
	}
	void Service::bind(const Viewport& viewport) {
		glViewport(viewport.position.x,viewport.position.y,viewport.size.x,viewport.size.y);
	}

	void Service::servicePreStep() {
#ifndef ARPHEG_RENDERING_GL_VAO
		//vaoEmulation_oldAttribEnabledCount = 0;
#endif
		currentPipeline = 0;
	}
}