#pragma once

#include "context.h"
#include "../types.h"
#include "../../core/memory.h"

#ifndef ARPHEG_RENDERING_GLES
	#define ARPHEG_RENDERING_GL_VAO 1
#endif

namespace rendering {

	class Service {
	public:
		enum { kMaxTextureSlots = 4 };

		Service();
		~Service();
		void servicePreStep();

		inline opengl::Context* context();

		//Buffer objects
		Buffer create (Buffer::Type type,bool dynamic,size_t size,void* data = nullptr);
		void   recreate(Buffer::Type type,Buffer buffer,bool dynamic,size_t size,void* data = nullptr);
		void   update (Buffer::Type type,Buffer buffer,size_t offset,void* data,size_t size);
		Buffer::Mapping map(Buffer::Type type,Buffer buffer);
		void   unmap(const Buffer::Mapping& mapping);
		void   release(Buffer buffer);

		Mesh   create(Buffer vertices,Buffer indices,const VertexDescriptor& vertexLayout);
		void   update(Mesh& mesh,Buffer vertices,Buffer indices,const VertexDescriptor& vertexLayout);
		void   release(const Mesh& mesh);

		//Texture and sampler objects
		bool verify(const texture::Descriptor2D& descriptor);
		TextureBuffer create(uint32 format,Buffer buffer);
		Texture1D create(const texture::Descriptor1D& descriptor,const void* texels = nullptr);
		Texture2D create(const texture::Descriptor2D& descriptor,const void* texels = nullptr);
		Texture2DArray create(const texture::Descriptor2D& descriptor,int numLayers,const void* texels = nullptr);
		Texture3D create(const texture::Descriptor3D& descriptor,const void* texels = nullptr);
		inline void generateMipmaps(Texture2DArray texture);
		inline void generateMipmaps(Texture2D texture);
		void load(Texture2D destination,vec2i offset,const texture::Descriptor2D& descriptor,void* texels);
		void load(Texture3D destination,const texture::Descriptor3D& descriptor,void* texels);
		void resize(Texture2D texture,const texture::Descriptor2D& descriptor);
		inline void release(TextureBuffer texture);
		inline void release(Texture1D texture);
		inline void release(Texture2D texture);
		inline void release(Texture2DArray texture);
		inline void release(Texture3D texture);

		Sampler create(texture::SamplerDescriptor& descriptor);
		void release(Sampler sampler);
		
		//Shader and pipeline objects
		Shader create(Shader::Type type,const char* src,size_t sourceLength = 0);
		void release(Shader shader);
		void release(Pipeline pipeline);
		Pipeline create(Shader shaders[],size_t count,const Pipeline::GLSLLayout& glslLayout = Pipeline::GLSLLayout());

		//Target surface management
		void clear(const vec4f& color = vec4f(0,0,0,0),bool clearColour = true,bool clearDepth = true,bool clearStencil = true);
		RenderTarget create(Texture2D* attachments,uint32 attachmentCount);
		void release(RenderTarget target);
		void bind(RenderTarget renderTarget);
		RenderTarget backBuffer() const;

		//Direct rendering access
		void bind(const Viewport& viewport);
		void bind(const blending::State& state);
		void bind(const rasterization::State& state);
		void bind(const Mesh& mesh,topology::Primitive primitiveTopology = topology::Triangle,uint32 indexSize = 0);

		void bind(Pipeline pipeline);
		void bind(Pipeline::Constant& constant,void* data);  //Direct uniform binding by value.
		void bind(Pipeline::Constant& constant,const mat44f& matrix);
		void bind(Pipeline::Constant& constant,const mat44f* matrix);
		void bind(Pipeline::Constant& constant,Buffer data,size_t offset = 0,size_t size = 0);
		
		inline void bind(TextureBuffer texture,uint32 slot = 0);
		inline void bind(Texture1D texture,uint32 slot = 0);
		inline void bind(Texture2D texture,uint32 slot = 0);
		inline void bind(Texture2DArray texture,uint32 slot = 0);
		inline void bind(Texture3D texture,uint32 slot = 0);
		void bind(Sampler sampler,uint32 slot = 0);

		void draw(uint32 offset,uint32 count);
		void drawIndexed(uint32 offset,uint32 count);
		void drawIndexed(uint32 offset,uint32 count,uint32 baseVertex);

	public:
		// State for the next draw invocation
		uint32 currentIndexSize,currentPrimitiveTopology;
		// Current program
		uint32 currentPipeline;
		// Vertex input state possibly used for the next draw invocation.
#ifndef ARPHEG_RENDERING_GL_VAO
		uint32 vaoEmulation_oldAttribEnabledCount;
#endif
		uint32 samplerEmulationSlots[kMaxTextureSlots];

		blending::State blendingState_;
		rasterization::State rasterState_;
		uint32 samplerEmulationDefault;
#ifndef ARPHEG_RENDERING_GL_VAO
		core::BufferAllocator vaoEmulationBuffer;
#endif
		core::BufferAllocator objectEmulationBuffer;
		core::BufferAllocator samplerEmulationBuffer;
		opengl::Context ctx;

	private:
		void bind(const Texture2D* texture,uint32 slot);
		void release(const Texture2D* texture);
		void generateMipmaps(const Texture2D* texture);
	};

	inline opengl::Context* Service::context() {
		return &ctx;
	}
	inline void Service::bind(TextureBuffer texture,uint32 slot) { bind((Texture2D*)(&texture),slot); }
	inline void Service::bind(Texture1D texture,uint32 slot) { bind((Texture2D*)(&texture),slot); }
	inline void Service::bind(Texture2D texture,uint32 slot) { bind((Texture2D*)(&texture),slot); }
	inline void Service::bind(Texture2DArray texture,uint32 slot) { bind((Texture2D*)(&texture),slot); }
	inline void Service::bind(Texture3D texture,uint32 slot) { bind((Texture2D*)(&texture),slot); }
	inline void Service::generateMipmaps(Texture2DArray texture) { generateMipmaps((Texture2D*)&texture); }
	inline void Service::generateMipmaps(Texture2D texture) { generateMipmaps((Texture2D*)&texture); }
	inline void Service::release(TextureBuffer texture) { release((Texture2D*)&texture); }
	inline void Service::release(Texture1D texture) { release((Texture2D*)&texture); }
	inline void Service::release(Texture2D texture) { release((Texture2D*)&texture); }
	inline void Service::release(Texture2DArray texture) { release((Texture2D*)&texture); }
	inline void Service::release(Texture3D texture) { release((Texture2D*)&texture); }
}
