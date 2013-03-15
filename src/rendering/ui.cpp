#include <limits>
#include <string.h>
#include "../core/assert.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../services.h"
#include "rendering.h"
#include "ui.h"
#include "../data/font/font.h"
#include "text.h"
#include "opengl/gl.h"

namespace rendering {
namespace ui {

Batch::Batch() : verticesSize(0),indicesSize(0),primitive(topology::Triangle),name("") {
}
struct BatchImpl {
	core::BufferAllocator vertices;
	core::BufferAllocator indices;
	uint32 vertexSize;
	uint32 meshId;
	//size_t maxVerticesSize,maxIndicesSize;

	BatchImpl(const Batch& batch);
};
BatchImpl::BatchImpl(const Batch& batch) : 
	vertices(batch.verticesSize?batch.verticesSize: 2048,services::threadSafeFrameAllocator(),core::BufferAllocator::GrowOnOverflow),
	indices(batch.indicesSize? batch.indicesSize:  1024,services::threadSafeFrameAllocator() ,core::BufferAllocator::GrowOnOverflow) 
{
	vertexSize = 0;
	for(uint32 i = 0;i<batch.vertexLayout.count;++i){
		vertexSize+=batch.vertexLayout.fields[i].size();
	}
	meshId = 0;
}
struct MeshImpl {
	VertexDescriptor vertexLayout;
	Buffer vbo;
	Buffer ibo;
	Mesh mesh;
	size_t requiredVertexSize;
	size_t requiredIndexSize;

	bool matches(const Batch& batch){
		if(vertexLayout.count != batch.vertexLayout.count) return false;
		for(uint32 i = 0;i<batch.vertexLayout.count;++i){
			if(!(vertexLayout.fields[i] == batch.vertexLayout.fields[i])) return false;
		}
		return true;
	}
};


uint32 Service::registerBatch(const Batch& batch,const Batch::Material& pipeline) {
	using namespace core::bufferArray;
	auto bimpl = new(batches_.allocate(sizeof(BatchImpl),alignof(BatchImpl))) BatchImpl(batch);
	add<Batch::Material>(batchPipelines_,pipeline);
	//Find an appropriate mesh (if any)
	uint32 meshId  = uint32(length<MeshImpl>(meshes_));
	for(uint32 i =0;i<meshId;++i){
		if(nth<MeshImpl>(meshes_,i).matches(batch)){
			meshId = i; break;
		}
	}
	if(meshId == length<MeshImpl>(meshes_)){
		//Create a new mesh.
		auto meshImpl =	core::bufferArray::allocate<MeshImpl>(meshes_);
		meshImpl->vertexLayout = batch.vertexLayout;
		meshImpl->vbo = Buffer::nullBuffer();
		meshImpl->ibo = Buffer::nullBuffer();
	}
	bimpl->meshId = meshId;
	return uint32(length<BatchImpl>(batches_)-1);
}

struct BatchFontMapping {
	const data::Font* key;
	uint32 value;
};

uint32 Service::registerFontBatch(const data::Font* font) {
	Batch batch;
	batch.vertexLayout = text::vertexLayout(font);
	batch.name = "font";
	Batch::Material material;
	material.pipeline = font->renderingType() == text::fontType::Outlined? defaultOutlinedFontPipeline_ : defaultFontPipeline_;
	assert(font->pageCount_ < Batch::Material::kMaxTextures);
	material.textureCount = font->pageCount_;
	for(uint32 i = 0;i< font->pageCount_;++i){
		material.textures[i].texture  = font->pages_[i];
		material.textures[i].constant = Pipeline::Constant("texture");
	}
	auto id = registerBatch(batch,material);
	BatchFontMapping map = {font,id};
	core::bufferArray::add<BatchFontMapping>(fontBatches_,map);
	return id;
}

Batch::Geometry Service::allocate(uint32 layerId,const data::Font* font,uint32 vertexCount,uint32 indexCount) {
	//Find the batch for this font
	uint32 batchId = 0xFFFF;
	using namespace core::bufferArray;
	for(const BatchFontMapping* i = begin<BatchFontMapping>(fontBatches_),*e = end<BatchFontMapping>(fontBatches_);i<e;++i){
		if(i->key == font){
			batchId = i->value;
			break;
		}
	}
	if(batchId == 0xFFFF) batchId = registerFontBatch(font);
	return allocate(layerId,batchId,vertexCount,indexCount);
}
Batch::Geometry Service::allocate(uint32 layerId,uint32 batchId,uint32 vertexCount,uint32 indexCount) {
	assert(batchId < core::bufferArray::length<BatchImpl>(batches_));
	auto batch = core::bufferArray::begin<BatchImpl>(batches_) + batchId;
	Batch::Geometry result = {
		batch->vertices.size()/batch->vertexSize,
		(float*)batch->vertices.allocate(batch->vertexSize*vertexCount),
		indexCount? (uint16*)batch->indices.allocate(sizeof(uint16)*indexCount) : nullptr
	};
	return result;
}

// Immediate mode rendering of ui primitives
void Service::render(const mat44f& matrix) {
	auto renderer = services::rendering();
	//Turn off blending
	renderer->bind(blending::alpha());
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	using namespace core::bufferArray;

	//Allocate the storage for mesh buffers
	struct MeshSupport {
		size_t requiredVertexSize;
		size_t requiredIndexSize;
		uint32 vertexOffset,indexOffset;//Offsets are in vertex/index size units
		Buffer::Mapping vertexMapping,indexMapping;
		bool justCreated;

		inline uint8* vertices(uint32 vertexSize){ return ((uint8*)vertexMapping.data) + vertexOffset*vertexSize; }
		inline uint8* indices(){ return ((uint8*)indexMapping.data) + indexOffset*sizeof(uint16); }
	};
	struct BatchSupport {
		bool draw,indexed;
		uint32 offset,count;
	};

	size_t meshCount = length<MeshImpl>(meshes_);
	MeshSupport* meshSupport = (MeshSupport*)services::frameAllocator()->allocate(sizeof(MeshSupport)*meshCount,alignof(MeshSupport));
	for(size_t i = 0;i<meshCount;++i){
		meshSupport[i].requiredVertexSize = meshSupport[i].requiredIndexSize = 0;
		meshSupport[i].justCreated = false;
	}
	for(BatchImpl* i = begin<BatchImpl>(batches_),*e = end<BatchImpl>(batches_);i<e;++i){
		meshSupport[i->meshId].requiredVertexSize+= i->vertices.size();
		meshSupport[i->meshId].requiredIndexSize += i->indices.size();
	}
	for(size_t i = 0;i<meshCount;++i){
		auto mesh = begin<MeshImpl>(meshes_)+i;
		if(mesh->vbo.isNull()){
			mesh->vbo = renderer->create(rendering::Buffer::Vertex,true,meshSupport[i].requiredVertexSize);
			mesh->ibo = renderer->create(rendering::Buffer::Index,true,meshSupport[i].requiredIndexSize);
			mesh->mesh = renderer->create(mesh->vbo,mesh->ibo,mesh->vertexLayout);
			meshSupport[i].justCreated = true;
		}
	}

	size_t batchCount = length<BatchImpl>(batches_);
	BatchSupport* batchSupport = (BatchSupport*)services::frameAllocator()->allocate(sizeof(BatchSupport)*batchCount,alignof(BatchSupport));
	
	//Update buffers - strategy : discard + map
	for(size_t i = 0;i<meshCount;++i){
		auto mesh = begin<MeshImpl>(meshes_)+i;
		if(!meshSupport[i].justCreated){
			renderer->recreate(rendering::Buffer::Vertex,mesh->vbo,true,meshSupport[i].requiredVertexSize);
			renderer->recreate(rendering::Buffer::Index,mesh->ibo,true,meshSupport[i].requiredIndexSize);
		}
		if(meshSupport[i].requiredVertexSize > 0){
			meshSupport[i].vertexMapping = renderer->map(rendering::Buffer::Vertex,mesh->vbo);
			meshSupport[i].vertexOffset = 0;
		}
		if(meshSupport[i].requiredIndexSize > 0){
			meshSupport[i].indexMapping  = renderer->map(rendering::Buffer::Index,mesh->ibo);
			meshSupport[i].indexOffset = 0;
		}
	}
	for(size_t i = 0;i<batchCount;++i){
		const BatchImpl* batch = begin<BatchImpl>(batches_) + i;
		BatchSupport support;
		support.draw = false;
		if(batch->vertices.size()){
			support.draw = true;
			memcpy(meshSupport[batch->meshId].vertices(batch->vertexSize),batch->vertices.bufferBase(),batch->vertices.size());
			support.offset = uint32(meshSupport[batch->meshId].vertexOffset);
			uint32 vertexCount = uint32(batch->vertices.size()/batch->vertexSize);
			support.count = vertexCount;
			meshSupport[batch->meshId].vertexOffset+=vertexCount;
		}
		if(batch->indices.size()){
			support.indexed = true;
			memcpy(meshSupport[batch->meshId].indices(),batch->indices.bufferBase(),batch->indices.size());
			support.offset = uint32(meshSupport[batch->meshId].indexOffset);//NB: important
			uint32 indexCount = uint32(batch->indices.size()/sizeof(uint16));
			support.count = indexCount;
			meshSupport[batch->meshId].indexOffset+=indexCount;
		}
		batchSupport[i] = support;
	}
	for(size_t i = 0;i<meshCount;++i){
		if(meshSupport[i].requiredVertexSize > 0)  renderer->unmap(meshSupport[i].vertexMapping);
		if(meshSupport[i].requiredIndexSize > 0)   renderer->unmap(meshSupport[i].indexMapping);
	}

	//Issue the rendering commands
	uint32 currentBoundMesh = uint32(meshCount);

	Pipeline currentBoundPipeline = Pipeline::nullPipeline();

	uint32 textureUnits = 8;

	for(size_t i = 0;i<batchCount;++i){
		if(!batchSupport[i].draw) continue;
		Batch::Material* material = begin<Batch::Material>(batchPipelines_) + i;
		if(!(material->pipeline == currentBoundPipeline)){
			currentBoundPipeline = material->pipeline;
			renderer->bind(material->pipeline);
			renderer->bind(material->matrix,&matrix);
		}
		//Bind textures.
		for(uint32 j = 0;j<material->textureCount;++j){
			renderer->bind(material->textures[j].texture,j);
			int target = j;
			renderer->bind(material->textures[j].constant,&target);
		}

		const BatchImpl* batch = begin<BatchImpl>(batches_) + i;
		//Bind mesh
		if(currentBoundMesh != batch->meshId){
			const MeshImpl* mesh = begin<MeshImpl>(meshes_) + batch->meshId;
			renderer->bind(mesh->mesh,rendering::topology::Triangle,sizeof(uint16));
			currentBoundMesh = batch->meshId;
		}

		if(batchSupport[i].indexed)
			renderer->drawIndexed(batchSupport[i].offset,batchSupport[i].count);
		else
			renderer->draw(batchSupport[i].offset,batchSupport[i].count);
	}

	glEnable(GL_DEPTH_TEST);
	renderer->bind(blending::disabled());
}
void Service::servicePreStep(){
	new(&fontBatches_)    core::BufferAllocator(sizeof(BatchFontMapping)*32,services::frameAllocator(),core::BufferAllocator::GrowOnOverflow);
	new(&batches_)        core::BufferAllocator(sizeof(BatchImpl)*64,services::frameAllocator(),core::BufferAllocator::GrowOnOverflow);
	new(&batchPipelines_) core::BufferAllocator(sizeof(Batch::Material)*64,services::frameAllocator(),core::BufferAllocator::GrowOnOverflow);
}
void Service::servicePostStep(){
}
void Service::registerFontPipelines(Pipeline text,Pipeline outlinedText) {
	defaultFontPipeline_ = text;
	defaultOutlinedFontPipeline_ = outlinedText;
}
Service::Service() :
meshes_(sizeof(MeshImpl)*64,nullptr,core::BufferAllocator::GrowOnOverflow),
batches_(128,services::frameAllocator()),
fontBatches_(128,services::frameAllocator()),
batchPipelines_(128,services::frameAllocator())
{
	defaultFontPipeline_ = Pipeline::nullPipeline();
	defaultOutlinedFontPipeline_ = Pipeline::nullPipeline();
}
Service::~Service(){
	using namespace core::bufferArray;
	auto renderer = services::rendering();
	for(MeshImpl* i = begin<MeshImpl>(meshes_),*e = end<MeshImpl>(meshes_);i<e;++i){
		if(!i->vbo.isNull()){
			renderer->release(i->mesh);
			renderer->release(i->vbo);
			if(!i->ibo.isNull()) renderer->release(i->ibo);
		}
	}
}

} }