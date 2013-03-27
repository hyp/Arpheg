#include <limits>
#include <string.h>
#include "../core/assert.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../services.h"
#include "rendering.h"
#include "ui.h"
#include "../data/font/font.h"
#include "2d.h"
#include "text.h"
#include "opengl/gl.h"

namespace rendering {
namespace ui {

enum {
	NormalGeometry, TextGeometry, OutlinedTextGeometry
};

Batch::Batch() : verticesSize(0),indicesSize(0),primitive(topology::Triangle),name("") {
	layer=depth = 0;
}
struct BatchImpl {
	uint32 layerDepth;
	core::BufferAllocator vertices;
	core::BufferAllocator indices;
	uint32 meshId;
	uint32 vertexSize;
	int pipelineId;
	const data::Font* font;
	uint32 textureId;
	//size_t maxVerticesSize,maxIndicesSize;

	BatchImpl(const Batch& batch);
	BatchImpl(const BatchImpl& other,uint32 layer,uint32 texture);

	inline bool operator < (const BatchImpl& other){
		return this->layerDepth < other.layerDepth;
	}
};
BatchImpl::BatchImpl(const Batch& batch) : 
	vertices(batch.verticesSize?batch.verticesSize: 2048,services::threadSafeFrameAllocator(),core::BufferAllocator::GrowOnOverflow),
	indices(batch.indicesSize? batch.indicesSize:  1024,services::threadSafeFrameAllocator() ,core::BufferAllocator::GrowOnOverflow) 
{
	assert(batch.layer <= Batch::kMaxLayer);
	assert(batch.depth <= Batch::kMaxDepth);
	layerDepth = (batch.layer<<8) | batch.depth;
	meshId = 0;
	font = nullptr;
	textureId = 0;
}
BatchImpl::BatchImpl(const BatchImpl& other,uint32 layer,uint32 texture) : 
	vertices(2048,services::threadSafeFrameAllocator(),core::BufferAllocator::GrowOnOverflow),
	indices( 1024,services::threadSafeFrameAllocator() ,core::BufferAllocator::GrowOnOverflow)
{
	assert(layer <= Batch::kMaxLayer);
	layerDepth = (layer << 8) | (other.layerDepth & Batch::kMaxDepth);
	meshId = other.meshId;
	font = other.font;
	textureId = texture;
	vertexSize = other.vertexSize;
	pipelineId = other.pipelineId;
}

uint32 Service::registerBatch(const Batch& batch,int pipelineId,uint32 meshId ) {
	using namespace core::bufferArray;
	auto bimpl = new(batches_.allocate(sizeof(BatchImpl),alignof(BatchImpl))) BatchImpl(batch);
	bimpl->meshId     = meshId;
	bimpl->vertexSize = meshes_[meshId].vertexSize;
	bimpl->pipelineId = pipelineId; 
	return uint32(length<BatchImpl>(batches_)-1);
}
uint32 Service::cloneBatch(uint32 id,uint32 layerId,uint32 textureId) {
	using namespace core::bufferArray;
	auto bimpl = new(batches_.allocate(sizeof(BatchImpl),alignof(BatchImpl))) BatchImpl(nth<BatchImpl>(batches_,id),layerId,textureId);
	return uint32(length<BatchImpl>(batches_)-1);
}

struct BatchFontMapping {
	const data::Font* key;
	uint32 value;
};


Service::UniquePipeline::UniquePipeline(Pipeline pipeline) :matrixConstant("matrix"),texturesConstant("textures") {
	this->pipeline = pipeline;
}
uint32 Service::uiPipeline(const rendering::Pipeline& pipeline) {
	for(uint32 i = 0;i<kMaxPipelines;++i){
		if(pipelines_[i].pipeline == pipeline) return i;
	}
	for(uint32 dest = kMaxDefaultPipelines;dest<kMaxPipelines;++dest){
		if(pipelines_[dest].pipeline == Pipeline::nullPipeline()){
			pipelines_[dest] = UniquePipeline(pipeline);
			return dest;
		}
	}
	assertRelease(false && "Can't register a new ui pipeline!");
	return 0;
}
uint32 Service::uiTexture (const rendering::Texture2D& texture) {
	for(uint32 i = 0;i<kMaxTextures;++i){
		if(textures_[i] == texture) return i;
	}
	for(uint32 dest = 0;dest<kMaxTextures;++dest){
		if(textures_[dest] == Texture2D::null()){
			textures_[dest] = texture;
			return dest;
		}
	}
	assertRelease(false && "Can't register a new ui texture - need mores storage units!");
	return 0;
}


bool Service::loadCorePipeline(int id,const char* name) {
	int& pipe = defaultPipelines_[id];
	if(pipe == -1){
		if(auto asset = services::data()->pipeline(services::data()->bundle("core",true),name,true)){
			pipe = id;
			assert(pipelines_[id].pipeline == Pipeline::nullPipeline());
			pipelines_[id] = UniquePipeline(asset->pipeline());
		}  else {
			pipe = -2;
			return false;
		}
	}
	return true;
}

uint32 Service::registerFontBatch(const data::Font* font) {
	if(font->renderingType() == text::fontType::Outlined) 
		loadCorePipeline(OutlinedTextPipeline,"rendering.text.outlined.pipeline");
	else
		loadCorePipeline(TextPipeline,"rendering.text.pipeline");

	Batch batch;
	batch.name = "font";
	batch.depth = Batch::kMaxDepth;//fonts are drawn after all eveything else on the current layer.

	auto id = registerBatch(batch,
		font->renderingType() == text::fontType::Outlined? OutlinedTextPipeline : TextPipeline,
		font->renderingType() == text::fontType::Outlined? OutlinedTextGeometry : TextGeometry);
	core::bufferArray::nth<BatchImpl>(batches_,id).font = font;
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
	return allocate(layerId,batchId,0,vertexCount,indexCount);
}
Batch::Geometry Service::allocate(uint32 layerId,uint32 batchId,uint32 textureId,uint32 vertexCount,uint32 indexCount) {
	assert(batchId < core::bufferArray::length<BatchImpl>(batches_));
	
	auto batch = core::bufferArray::begin<BatchImpl>(batches_) + batchId;
	if(batch->textureId != textureId){
		batch = core::bufferArray::begin<BatchImpl>(batches_) + cloneBatch(batchId,layerId,textureId);
	}

	Batch::Geometry result = {
		batch->vertices.size()/batch->vertexSize,
		(float*)batch->vertices.allocate(batch->vertexSize*vertexCount),
		indexCount? (uint16*)batch->indices.allocate(sizeof(uint16)*indexCount) : nullptr
	};
	return result;
}

static VertexDescriptor meshVertexDescriptor(uint32 i){
	return i == NormalGeometry? draw2D::positionInt16::vertexLayout(draw2D::mode::Textured|draw2D::mode::Coloured) : 
		   i == TextGeometry?   text::vertexLayout(text::fontType::Default) : text::vertexLayout(text::fontType::Outlined) ;
}
static uint32 vertexSize(uint32 i){
	auto layout = meshVertexDescriptor(i);
	uint32 sz  = 0;
	for(uint32 i = 0;i<layout.count;++i){
		sz += layout.fields[i].size();
	}
	return sz;
}

void Service::prepareRendering() {
	using namespace core::bufferArray;

	//Get default pipelines.
	loadCorePipeline(TexturedColouredPipeline,"rendering.2d.textured.coloured.pipeline");
	loadCorePipeline(ColouredPipeline,"rendering.2d.coloured.pipeline");

	pointSampler_ = services::data()->sampler(services::data()->bundle("core",true),"rendering.sampler.point",true);
	fontSampler_ = pointSampler_;
	
	//Sort the batches.
	auto batchCount = length<BatchImpl>(batches_);
	uint32 storage[1024];
	if(batchCount > (sizeof(storage)/sizeof(storage[0]))){
		assert(false && "Can't sort the ui batches");
		return;
	}
	for(size_t i = 0;i<batchCount;++i){
		storage[i] = (uint32(i)<<16) | nth<BatchImpl>(batches_,i).layerDepth;
	}
	struct Pred{ inline bool operator ()(uint32 a,uint32 b) { return (a&0xFFFF) < (b&0xFFFF); } };
	std::sort(storage,storage+batchCount,Pred());

	//TODO sorted indexing.
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
	MeshSupport meshSupport[kMaxGeometries];

	struct BatchSupport {
		bool draw,indexed;
		uint32 baseVertex;
		uint32 offset,count;
	};
	
	for(uint32 i = 0;i<kMaxGeometries;++i){
		meshSupport[i].requiredVertexSize = meshSupport[i].requiredIndexSize = 0;
		meshSupport[i].justCreated = false;
	}
	for(BatchImpl* i = begin<BatchImpl>(batches_),*e = end<BatchImpl>(batches_);i<e;++i){
		meshSupport[i->meshId].requiredVertexSize+= i->vertices.size();
		meshSupport[i->meshId].requiredIndexSize += i->indices.size();
	}
	for(uint32 i = 0;i<kMaxGeometries;++i){
		auto mesh = meshes_ + i;
		if(mesh->vbo.isNull()){
			mesh->vbo = renderer->create(rendering::Buffer::Vertex,true,meshSupport[i].requiredVertexSize);
			mesh->ibo = renderer->create(rendering::Buffer::Index,true,meshSupport[i].requiredIndexSize);
			mesh->mesh = renderer->create(mesh->vbo,mesh->ibo,meshVertexDescriptor(i));
			meshSupport[i].justCreated = true;
		}
	}

	size_t batchCount = length<BatchImpl>(batches_);
	BatchSupport* batchSupport = (BatchSupport*)services::frameAllocator()->allocate(sizeof(BatchSupport)*batchCount,alignof(BatchSupport));
	
	//Update buffers - strategy : discard + map
	for(uint32 i = 0;i<kMaxGeometries;++i){
		auto mesh = meshes_ + i;
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
			support.baseVertex = meshSupport[batch->meshId].vertexOffset;
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
			support.offset = uint32(meshSupport[batch->meshId].indexOffset)*sizeof(uint16);//NB: important
			uint32 indexCount = uint32(batch->indices.size()/sizeof(uint16));
			support.count = indexCount;
			meshSupport[batch->meshId].indexOffset+=indexCount;
		}
		batchSupport[i] = support;
	}
	for(uint32 i = 0;i<kMaxGeometries;++i){
		if(meshSupport[i].requiredVertexSize > 0)  renderer->unmap(meshSupport[i].vertexMapping);
		if(meshSupport[i].requiredIndexSize > 0)   renderer->unmap(meshSupport[i].indexMapping);
	}

	//Setup textures.
	if(!(textures_[0] == Texture2D::null())){
		renderer->bind(textures_[0],0);
		renderer->bind(pointSampler_,0);
	}

	//Issue the rendering commands
	uint32 currentBoundMesh = kMaxGeometries;
	Pipeline currentBoundPipeline = Pipeline::nullPipeline();
	bool constantsBound[kMaxPipelines] = {false};
	bool textureUnitsBound[kMaxPipelines] = {false};

	for(size_t i = 0;i<batchCount;++i){
		if(!batchSupport[i].draw) continue;

		const BatchImpl* batch = begin<BatchImpl>(batches_) + i;

		//Bind Pipeline.
		if(!(pipelines_[batch->pipelineId].pipeline == currentBoundPipeline)){
			renderer->bind(pipelines_[batch->pipelineId].pipeline);
			if(!constantsBound[batch->pipelineId]){
				renderer->bind(pipelines_[batch->pipelineId].matrixConstant,matrix);
				constantsBound[batch->pipelineId] = true; //Reuse the constants.
			}
			if(batch->font){
				int fontTextureUnit = 2;
				renderer->bind(batch->font->pages_[0],fontTextureUnit);
				renderer->bind(fontSampler_,fontTextureUnit);
				renderer->bind(pipelines_[batch->pipelineId].texturesConstant,&fontTextureUnit);
			} else {
				if(batch->textureId != 0){
					int unit = 1;
					renderer->bind(textures_[batch->textureId],unit);
					renderer->bind(pointSampler_,unit);
					renderer->bind(pipelines_[batch->pipelineId].texturesConstant,&unit);
					textureUnitsBound[batch->pipelineId] = false;
				} else if(!textureUnitsBound[batch->pipelineId]) {
					int unit = 0 ;
					renderer->bind(pipelines_[batch->pipelineId].texturesConstant,&unit);
					textureUnitsBound[batch->pipelineId] = true;
				}
			}
		}
		

		//Bind mesh
		if(currentBoundMesh != batch->meshId){
			renderer->bind(meshes_[batch->meshId].mesh,rendering::topology::Triangle,sizeof(uint16));
			currentBoundMesh = batch->meshId;
		}

		if(batchSupport[i].indexed)
			renderer->drawIndexed(batchSupport[i].offset,batchSupport[i].count,batchSupport[i].baseVertex);
		else
			renderer->draw(batchSupport[i].offset,batchSupport[i].count);
	}

	glEnable(GL_DEPTH_TEST);
	renderer->bind(blending::disabled());
}
void Service::servicePreStep(){
	new(&fontBatches_)    core::BufferAllocator(sizeof(BatchFontMapping)*32,services::frameAllocator(),core::BufferAllocator::GrowOnOverflow);
	new(&batches_)        core::BufferAllocator(sizeof(BatchImpl)*64,services::frameAllocator(),core::BufferAllocator::GrowOnOverflow);

	//Release textures.
	for(uint32 dest = 0;dest<kMaxTextures;++dest){
		textures_[dest] = Texture2D::null();
	}
}
void Service::servicePostStep(){
}
Service::Service() :
batches_(128,services::frameAllocator()),
fontBatches_(128,services::frameAllocator())
{
	for(uint32 i = 0;i<kMaxDefaultPipelines;++i) defaultPipelines_[i] = -1;

	for(uint32 i = 0; i<kMaxGeometries;++i){
		meshes_[i].vertexSize = vertexSize(i);
		meshes_[i].vbo = meshes_[i].ibo = Buffer::nullBuffer();
	}
}
Service::~Service(){
	using namespace core::bufferArray;
	auto renderer = services::rendering();

	for(uint32 i = 0; i<kMaxGeometries;++i){
		if(!meshes_[i].vbo.isNull()){
			renderer->release(meshes_[i].mesh);
			renderer->release(meshes_[i].vbo);
			if(meshes_[i].ibo.isNull()) renderer->release(meshes_[i].ibo);
		}
	}
}

} }