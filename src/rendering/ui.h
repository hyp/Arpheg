// Provides a single threaded, multi-layered and 
// multi-batchable(each batch can have different vertex format and shader)
// service for rendering of ui elements.

#pragma once

#include "types.h"
#include "../core/math.h"
#include "../core/memory.h"
#include "../data/types.h"

namespace rendering {
namespace ui {

struct Batch {
	typedef batching::Geometry Geometry;

	struct Data {
		uint64 bytes[32/sizeof(uint64)];
	};
	struct Material {
		Pipeline pipeline;
		Pipeline::Constant matrix;
		uint32 textureCount; 
		bool residentTextures;
		struct Texture {
			Texture2D texture;
			Pipeline::Constant constant;

			Texture():constant("") {}
		};
		enum { kMaxTextures = 4 };
		Texture textures[kMaxTextures];

		Material():matrix("matrix"),textureCount(0),residentTextures(false) {}
	};

	VertexDescriptor vertexLayout;
	size_t verticesSize,indicesSize;
	topology::Primitive primitive;
	const char* name;

	Batch();
};

class Service {
public:

	Batch::Geometry allocate(uint32 layerId,const data::Font* font,uint32 vertexCount,uint32 indexCount);
	Batch::Geometry allocate(uint32 layerId,uint32 batchId,uint32 vertexCount,uint32 indexCount);

	//The register functions have to be called per frame.
	void registerFontPipelines(Pipeline text,Pipeline outlinedText);
	uint32 registerBatch(const Batch& batch,const Batch::Material& material);

	void render(const mat44f& matrix);
	void servicePreStep();
	void servicePostStep();
	Service();
	~Service();
private:
	uint32 registerFontBatch(const data::Font* font);
	core::BufferAllocator batches_;
	core::BufferAllocator fontBatches_;
	Pipeline defaultFontPipeline_, defaultOutlinedFontPipeline_;
	core::BufferAllocator batchPipelines_;
	core::BufferAllocator meshes_;
};

} }