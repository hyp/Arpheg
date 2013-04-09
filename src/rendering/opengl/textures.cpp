/**
Implements a texture + sampler object backend for OpenGL(ES).
# Notes:
* OpenGL ES 2 doesn't support sampler objects, so they are emulated at extra run time low, typically non existant(see below) cost. 
  NB: The emulation is low cost because it doesn't really emulate the feature set - 
    it only binds a texture with a sampler once for the first time texture is bound.
	Therefore you can only bind one sampler to a texture.
	But when all textures were bound to samplers, there is basically no performance penalty at all!
* OpenGL ES 2 supports NPOT textures only with clamping and no mipmaps (An extension exists).
* OpenGL ES 2 only 2D textures are supported (An extension exists).
* OpenGL ES 2 - "Fuck compressed textures!".
* OpenGL 3: (sampler object are _partially_ supported). Considerations: keep the emulator just in case GPU doesn't support it.

# How does the sampler object emulation work?
Renderer contains two buffers for objects and textures.
When a sampler is created it is stored in an object buffer and its offset in the buffer is returned as id.
When a texture is created it is added to the texture buffer.
When a texture is being bind - a texture buffer is checked if it contains the given texture, and if it does, we bind the current sampler to the texture.

TODO: ES2: sampler object emulation - npot verification + mipmaps.
*/
#include "../../core/assert.h"
#include "../../core/allocatorNew.h"
#include "../../core/branchHint.h"
#include "../../services.h"
#include "gl.h"
#include "rendering.h"

#define EMULATE_SAMPLER_OBJECTS

namespace rendering {

#ifdef EMULATE_SAMPLER_OBJECTS
	struct EmulatedSamplerObject {
		texture::SamplerDescriptor descriptor;
		GLint minFilter,magFilter;
		GLint wrap[3];

		void bind(GLenum target,bool textureHasMipmaps = false){
			glTexParameteri ( target, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri ( target, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexParameteri ( target, GL_TEXTURE_WRAP_S, wrap[0] );
			glTexParameteri ( target, GL_TEXTURE_WRAP_T, wrap[1] );
			glTexParameteri ( target, GL_TEXTURE_WRAP_R, wrap[2] );
		}
	};
	static void markTextureNoSamplerBound(core::BufferAllocator& allocator,uint32 id){
		auto start = (uint32*)allocator.toPointer(0);
		auto end   = (uint32*)allocator.toPointer(allocator.size());
		//Reuse buffer space
		for(;start < end;start+=2){
			if(*start == 0){
				*start = id; return;
			}
		}
		auto ptr = (uint32*)allocator.allocate(sizeof(uint32)*2);
		*ptr = id;
	}
	static void bindTextureToSampler(GLenum target,core::BufferAllocator& allocator,uint32 id,EmulatedSamplerObject* sampler){
		auto start = (uint32*)allocator.toPointer(0);
		auto end   = (uint32*)allocator.toPointer(allocator.size());
		//Reuse buffer space
		for(;start < end;start+=2){
			if(*start == id){
				sampler->bind(target);
				*start = 0;
				break;
			}
		}
	}
	static void textureReleased(core::BufferAllocator& allocator,uint32 id){
		auto start = (uint32*)allocator.toPointer(0);
		auto end   = (uint32*)allocator.toPointer(allocator.size());
		//Reuse buffer space
		for(;start < end;start+=2){
			if(*start == id){
				*start = 0;
				break;
			}
		}
	}
#endif

	static inline bool isPowerOfTwo(int32 x){
		return (x & (x - 1)) == 0;
	}
	static inline GLint samplerWrapMode(uint32 mode){
		return mode == texture::SamplerDescriptor::Clamp? GL_CLAMP_TO_EDGE : (mode == texture::SamplerDescriptor::Repeat? GL_REPEAT : GL_MIRRORED_REPEAT);
	}

	//TODO: Mipmapping support
	Sampler Service::create(texture::SamplerDescriptor& descriptor) {
		using namespace texture;

#ifdef EMULATE_SAMPLER_OBJECTS
		EmulatedSamplerObject* samplerObject = ALLOCATOR_NEW(&objectEmulationBuffer,EmulatedSamplerObject);
		samplerObject->descriptor = descriptor;
		if(descriptor.filter & SamplerDescriptor::MinLinear){
			if(descriptor.filter & SamplerDescriptor::MipLinear) samplerObject->minFilter = GL_LINEAR_MIPMAP_LINEAR;
			else samplerObject->minFilter = GL_LINEAR;
		}
		else samplerObject->minFilter = GL_NEAREST;
		samplerObject->magFilter = descriptor.filter & SamplerDescriptor::MagLinear? GL_LINEAR: GL_NEAREST;
		samplerObject->wrap[0] = samplerWrapMode(descriptor.adressModes[0]);
		samplerObject->wrap[1] = samplerWrapMode(descriptor.adressModes[1]);
		samplerObject->wrap[2] = samplerWrapMode(descriptor.adressModes[2]);

		Sampler sampler = { objectEmulationBuffer.toOffset(samplerObject) };
		return sampler;
#else
#endif
	}
	void Service::release(Sampler sampler) {
		//Leave it hanging, emulating...
	}

	bool Service::verify(const texture::Descriptor2D& descriptor) {
#ifdef ARPHEG_RENDERING_GLES
		//TODO: re-enable?
		//NB: GLES 2 specification: for non-power of 2 textures, mipmapping and other modes than clamping aren't supported
		//unless an OES_texture_npot is present
		/*if(!isPowerOfTwo(descriptor.width) || !isPowerOfTwo(descriptor.height)){
			if( (descriptor.flags & texture::Clamp)==0 || (descriptor.flags & texture::NoMipmaps)==0 ){
				if(!context()->extensionSupported(opengl::extensions::OES_texture_npot)){
					services::application()->runtimeError("Unsupported NPOT texture!");
					return false;
				}
			}
		}*/
#endif
		return true;
	}
	static GLint textureFormat(uint32 format,GLint& internalFormat,GLint& type){
		using namespace texture;
		GLint result;
		switch(format){
		case R_8:    result = GL_RED; internalFormat = GL_R8; type = GL_UNSIGNED_BYTE; break;
		case RG_88:   result = GL_RG; internalFormat = GL_RG8;type = GL_UNSIGNED_BYTE; break;
		case RGB_888: result = GL_RGB;internalFormat = GL_RGB8; type = GL_UNSIGNED_BYTE; break;
		case RGBA_8888: result = GL_RGBA; internalFormat = GL_RGBA8; type = GL_UNSIGNED_BYTE; break;
		case UINT_R_16: result = GL_RED; internalFormat = GL_R16UI; type = GL_UNSIGNED_SHORT; break;
		case UINT_RG_1616: result = GL_RG; internalFormat = GL_RG16UI; type = GL_UNSIGNED_SHORT; break;
		case UINT_RGB_161616:result = GL_RGB; internalFormat = GL_RGB16UI; type = GL_UNSIGNED_SHORT; break;
		case UINT_RGBA_16161616:result = GL_RGBA; internalFormat = GL_RGBA16UI;  type = GL_UNSIGNED_SHORT;break;
		case UINT_R_32: result = GL_RED; internalFormat = GL_R32UI; type = GL_UNSIGNED_INT; break;
		case UINT_RG_3232: result = GL_RG; internalFormat = GL_RG32UI; type = GL_UNSIGNED_INT; break;
		case UINT_RGB_323232: result = GL_RGB; internalFormat = GL_RGB32UI; type = GL_UNSIGNED_INT; break;
		case UINT_RGBA_32323232: result = GL_RGBA; internalFormat = GL_RGBA32UI; type = GL_UNSIGNED_INT; break;
		case FLOAT_R_32: result = GL_RED; internalFormat = GL_R32F; type = GL_FLOAT; break;
		case FLOAT_RG_3232: result = GL_RG;  internalFormat = GL_RG32F; type = GL_FLOAT; break;
		case FLOAT_RGB_323232: result = GL_RGB; internalFormat = GL_RGB32F; type = GL_FLOAT; break;
		case FLOAT_RGBA_32323232: result = GL_RGBA;  internalFormat = GL_RGBA32F; type = GL_FLOAT; break;
		}
		return result;
	}
	//NB: check https://groups.google.com/forum/?fromgroups=#!topic/mac-opengl/ZSUKqH4RPTA
	Texture2D Service::create(const texture::Descriptor2D& descriptor,const void* texels) {
		verify(descriptor);
		Texture2D texture={0,GL_TEXTURE_2D};
		CHECK_GL(glGenTextures(1,&texture.id));
		assert(texture.id!=0);
		CHECK_GL(glBindTexture(GL_TEXTURE_2D, texture.id));
		bool rt = (descriptor.format & texture::RENDER_TARGET)!=0;
#ifdef EMULATE_SAMPLER_OBJECTS
		((EmulatedSamplerObject*)objectEmulationBuffer.toPointer(samplerEmulationDefault))->bind(GL_TEXTURE_2D);
		if(!rt) markTextureNoSamplerBound(samplerEmulationBuffer,texture.id);
#endif		
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0,internalFormat, descriptor.width, descriptor.height, 0, format, type, texels));
		return texture;
	}
	Texture2DArray Service::create(const texture::Descriptor2D& descriptor,int numLayers,const void* texels) {
		verify(descriptor);
		Texture2DArray texture={0,GL_TEXTURE_2D_ARRAY};
		CHECK_GL(glGenTextures(1,&texture.id));
		assert(texture.id!=0);
		CHECK_GL(glBindTexture(GL_TEXTURE_2D_ARRAY, texture.id));
#ifdef EMULATE_SAMPLER_OBJECTS
		((EmulatedSamplerObject*)objectEmulationBuffer.toPointer(samplerEmulationDefault))->bind(GL_TEXTURE_2D_ARRAY);
		markTextureNoSamplerBound(samplerEmulationBuffer,texture.id);
#endif		
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,internalFormat, descriptor.width, descriptor.height, numLayers, 0, format, type, texels));
		return texture;
	}
	TextureBuffer Service::create(uint32 format,Buffer buffer) {
		TextureBuffer texture={0,GL_TEXTURE_BUFFER};
		CHECK_GL(glGenTextures(1,&texture.id));
		assert(texture.id!=0);
		GLint internalFormat,type;
		GLint fmt = textureFormat(format,internalFormat,type);
		CHECK_GL(glBindTexture(GL_TEXTURE_BUFFER, texture.id));
		glTexBuffer(GL_TEXTURE_BUFFER,internalFormat,buffer.id);
		return texture;
	}
	Texture1D Service::create(const texture::Descriptor1D& descriptor,const void* texels) {
		Texture1D texture={0,GL_TEXTURE_1D};
		CHECK_GL(glGenTextures(1,&texture.id));
		assert(texture.id!=0);
		CHECK_GL(glBindTexture(GL_TEXTURE_1D, texture.id));
#ifdef EMULATE_SAMPLER_OBJECTS
		((EmulatedSamplerObject*)objectEmulationBuffer.toPointer(samplerEmulationDefault))->bind(GL_TEXTURE_1D);
		markTextureNoSamplerBound(samplerEmulationBuffer,texture.id);
#endif		
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexImage1D(GL_TEXTURE_1D, 0,internalFormat, descriptor.size, 0, format, type, texels));
		return texture;
	}
	Texture3D Service::create(const texture::Descriptor3D& descriptor,const void* texels) {
		Texture3D texture={0,GL_TEXTURE_3D};
		CHECK_GL(glGenTextures(1,&texture.id));
		assert(texture.id!=0);
		CHECK_GL(glBindTexture(GL_TEXTURE_3D, texture.id));
#ifdef EMULATE_SAMPLER_OBJECTS
		((EmulatedSamplerObject*)objectEmulationBuffer.toPointer(samplerEmulationDefault))->bind(GL_TEXTURE_3D);
		markTextureNoSamplerBound(samplerEmulationBuffer,texture.id);
#endif		
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexImage3D(GL_TEXTURE_3D, 0,internalFormat, descriptor.width, descriptor.height,descriptor.length, 0, GL_RG, type, texels));
		return texture;
	}

	void Service::load(Texture2D destination,vec2i offset,const texture::Descriptor2D& descriptor,void* texels){
		assert(texels);
		assert(destination.id!=0);
		//assert(destination.type == GL_TEXTURE_2D);

		CHECK_GL(glBindTexture(GL_TEXTURE_2D, destination.id));
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x,offset.y,descriptor.width,descriptor.height, format, type, texels));
	}
	void Service::load(Texture3D destination,const texture::Descriptor3D& descriptor,void* texels){
		assert(texels);
		assert(destination.id!=0);
		//assert(destination.type == GL_TEXTURE_3D || destination.type == GL_TEXTURE_2D_ARRAY);

		CHECK_GL(glBindTexture(GL_TEXTURE_3D, destination.id));
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexSubImage3D(GL_TEXTURE_3D, 0, 0,0,0 ,descriptor.width,descriptor.height,descriptor.length, GL_RG, type, texels));
	}
	void Service::resize(Texture2D texture,const texture::Descriptor2D& descriptor){
		assert(texture.id!=0);

		CHECK_GL(glBindTexture(GL_TEXTURE_2D, texture.id));
		GLint internalFormat,type;
		GLint format = textureFormat(descriptor.format,internalFormat,type);
		CHECK_GL(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,descriptor.width,descriptor.height, 0,format, type,nullptr));
	}

	void Service::generateMipmaps(const Texture2D* texture){
		assert(texture->type == GL_TEXTURE_2D || texture->type == GL_TEXTURE_2D_ARRAY);
		glBindTexture(texture->type,texture->id);
		CHECK_GL(glGenerateMipmap(texture->type));
	}
	void Service::release(const Texture2D* texture){
		assert(texture->id!=0);
		CHECK_GL(glDeleteTextures(1,&texture->id));
#ifdef EMULATE_SAMPLER_OBJECTS
		if(texture->type == GL_TEXTURE_BUFFER) return;
		if(LIKELY_FALSE(samplerEmulationBuffer.size() != 0))
			textureReleased(samplerEmulationBuffer,texture->id);
#endif
	}
	void Service::bind(const Texture2D* texture,uint32 slot){
		assert(slot < kMaxTextureSlots);
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(texture->type,texture->id);
		
#ifdef EMULATE_SAMPLER_OBJECTS
		if(texture->type == GL_TEXTURE_BUFFER) return;

		if(LIKELY_FALSE(samplerEmulationBuffer.size() != 0)) 
			bindTextureToSampler(texture->type,samplerEmulationBuffer,texture->id,(EmulatedSamplerObject*)objectEmulationBuffer.toPointer(samplerEmulationSlots[slot]));
#endif 
	}
	void Service::bind(Sampler sampler,uint32 slot) {
		assert(slot < kMaxTextureSlots);
#ifdef EMULATE_SAMPLER_OBJECTS
		samplerEmulationSlots[slot] = sampler.id;
#endif
	}

	RenderTarget Service::create(Texture2D* attachments, uint32 attachmentCount){
		RenderTarget target = { 0,0 };
		assert(attachmentCount != 0);

		CHECK_GL(glGenFramebuffers( 1, &target.id ));
		assert(target.id != 0);
		CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, target.id));

		glActiveTexture(GL_TEXTURE0);
		for(uint32 i = 0; i < attachmentCount; i++){
			glBindTexture(GL_TEXTURE_2D, attachments[i].id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			GLenum attachment = GL_COLOR_ATTACHMENT0 + i;//GL_DEPTH_ATTACHMENT
			CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, attachments[i].id, 0));
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Check FBO status.    
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if ( status != GL_FRAMEBUFFER_COMPLETE ) {
			services::logging()->critical("OpenGL: framebuffer not complete!");
		}
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return target;
	}
	void Service::release(RenderTarget target){
		assert(target.id != 0);
		glDeleteFramebuffers(1,(GLuint*)(&target.id));
	}
	RenderTarget Service::backBuffer() const {
		RenderTarget t = { 0,0 };
		return t;
	}
	void Service::bind(RenderTarget renderTarget){
		glBindFramebuffer(GL_FRAMEBUFFER, renderTarget.id);
	}


}
