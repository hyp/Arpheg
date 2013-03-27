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
	enum { kMaxLayer = 0xFF };
	enum { kMaxDepth = 0xFF };

	typedef batching::Geometry Geometry;

	size_t verticesSize,indicesSize;
	topology::Primitive primitive;
	const char* name;
	uint32 layer,depth;//Batches are sorted by depth and layers.

	Batch();
};

class Service {
public:
	enum {
		TexturedColouredPipeline,
		ColouredPipeline,
		TextPipeline,
		OutlinedTextPipeline,
		kMaxDefaultPipelines,
	};

	uint32 uiPipeline(const rendering::Pipeline& pipeline);
	uint32 uiTexture (const rendering::Texture2D& texture);
	Batch::Geometry allocate(uint32 layerId,const data::Font* font,uint32 vertexCount,uint32 indexCount);
	Batch::Geometry allocate(uint32 layerId,uint32 batchId,uint32 textureId,uint32 vertexCount,uint32 indexCount);
	uint32 registerBatch(const Batch& batch,int pipelineId,uint32 meshId = 0);

	void prepareRendering();
	void render(const mat44f& matrix);
	void servicePreStep();
	void servicePostStep();
	Service();
	~Service();
private:
	uint32 cloneBatch(uint32 id,uint32 layerId,uint32 textureId);
	bool loadCorePipeline(int id,const char* name);

	uint32 registerFontBatch(const data::Font* font);
	core::BufferAllocator batches_;
	core::BufferAllocator fontBatches_;

	enum {
		kMaxGeometries = 3
	};
	struct Mesh {
		uint32 vertexSize;
		rendering::Buffer vbo;
		rendering::Buffer ibo;
		rendering::Mesh   mesh;
	};
	Mesh meshes_[kMaxGeometries];

	struct UniquePipeline {
		Pipeline pipeline;
		Pipeline::Constant matrixConstant;
		Pipeline::Constant texturesConstant;

		UniquePipeline(Pipeline pipeline = Pipeline::nullPipeline());
	};
	enum { kMaxPipelines = 8 };
	UniquePipeline pipelines_[kMaxPipelines];
	int defaultPipelines_[kMaxDefaultPipelines];

	enum { kMaxTextures = 8 };
	rendering::Texture2D textures_[kMaxTextures];

	rendering::Sampler pointSampler_;
	rendering::Sampler fontSampler_;
};

} }