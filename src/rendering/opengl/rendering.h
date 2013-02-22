#pragma once

#include "context.h"
#include "../types.h"
#include "../../core/memory.h"

namespace rendering {

	class Service {
	public:
		enum { kMaxTextureSlots = 4 };

		Service();
		~Service();

		inline opengl::Context* context();

		//Buffer objects
		Buffer create (Buffer::Type type,bool dynamic,size_t size,void* data = nullptr);
		void   update (Buffer::Type type,Buffer buffer,size_t offset,void* data,size_t size);
		void   release(Buffer buffer);

		//Texture and sampler objects
		bool verify(const texture::Descriptor2D& descriptor);
		TextureBuffer create(uint32 format,Buffer buffer);
		Texture1D create(const texture::Descriptor1D& descriptor,const void* texels = nullptr);
		Texture2D create(const texture::Descriptor2D& descriptor,const void* texels = nullptr);
		Texture2DArray create(const texture::Descriptor2D& descriptor,int numLayers,const void* texels = nullptr);
		Texture3D create(const texture::Descriptor3D& descriptor,const void* texels = nullptr);
		void generateMipmaps(Texture2DArray texture);
		void load(Texture2D destination,vec2i offset,const texture::Descriptor2D& descriptor,void* texels);
		void load(Texture3D destination,const texture::Descriptor3D& descriptor,void* texels);
		void resize(Texture2D texture,texture::Descriptor2D& descriptor);
		void release(TextureBuffer texture);
		void release(Texture1D texture);
		void release(Texture2D texture);
		void release(Texture2DArray texture);
		void release(Texture3D texture);
		Sampler create(texture::SamplerDescriptor& descriptor);
		void release(Sampler sampler);
		
		//Shader and pipeline objects
		Shader create(Shader::Type type,const char* src,size_t sourceLength = 0);
		void release(Shader shader);
		Pipeline create(VertexDescriptor input,Shader vertexShader,Shader pixelShader);
		Pipeline create(VertexDescriptor input,Shader shaders[],size_t count);
		void release(Pipeline pipeline);

		//Target surface management
		void clear();
		void clear(const vec4f& color);
		RenderTarget create(Texture2D* attachments,uint32 attachmentCount);
		void release(RenderTarget target);
		void bind(RenderTarget renderTarget);
		RenderTarget backBuffer() const;

		//Direct rendering access
		void bind(const Viewport& viewport);
		void bind(const blending::State& state);
		void bind(const rasterization::State& state);
		

		void bind(topology::Primitive primitiveTopology);
		void bindVertices(Buffer buffer);
		void bindIndices (Buffer buffer,uint32 indexSize);
#ifndef PLATFORM_RENDERING_BUFFERS_ONLY
		void bindVerticesIndicesStream(void* vertices,void* indices = nullptr);
#endif

		void bind(Pipeline pipeline);
		void bind(Pipeline::Constant& constant,void* data);  //Direct uniform binding by value.
		void bind(Pipeline::Constant& constant,const mat44f& matrix);
		void bind(Pipeline::Constant& constant,Buffer data,size_t offset = 0,size_t size = 0);

		void bind(TextureBuffer texture,uint32 slot = 0);
		void bind(Texture1D texture,uint32 slot = 0);
		void bind(Texture2D texture,uint32 slot = 0);
		void bind(Texture2DArray texture,uint32 slot = 0);
		void bind(Texture3D texture,uint32 slot = 0);
		void bind(Sampler sampler,uint32 slot = 0);

		void draw(uint32 offset,uint32 count);
		void drawIndexed(uint32 offset,uint32 count);
		
		void servicePreStep();
	public:
		uint32 inputDescriptorEnabled;
		uint32 currentPipeline;
		uint32 currentPrimitiveTopology, currentIndexSize;
#ifndef PLATFORM_RENDERING_BUFFERS_ONLY
		void* inputVertices,*inputIndices;
#endif
		bool inputDescriptorBound;
		//pipeline's input descriptor
		uint32 inputDescriptorCount;
		CoreTypeDescriptor* inputDescriptor;
		uint32 samplerEmulationSlots[kMaxTextureSlots];

		blending::State blendingState;
		rasterization::State rasterState;
		uint32 samplerEmulationDefault;
		core::BufferAllocator objectEmulationBuffer;
		core::BufferAllocator samplerEmulationBuffer;
		opengl::Context ctx;
	};

	inline opengl::Context* Service::context() {
		return &ctx;
	}
}
