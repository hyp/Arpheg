#include <string.h>
#include "../../../core/memory.h"
#include "../../../core/bufferStringStream.h"
#include "../../../core/text.h"
#include "../../../core/io.h"
#include "../../../core/allocatorNew.h"
#include "../../../core/assert.h"
#include "../../../services.h"

#include "../../internal/internal.h"


#include "../../image/reader.h"
#include "../../shader/preprocess.h"
#include "../../utils/path.h"
#include "../mesh/reader.h"
#include "../font/reader.h"

#include "parser.h"

namespace data {
namespace intermediate {
namespace bundle {

void SubParser::begin() {
}
void SubParser::set(core::Bytes id){
}
text::Parser::SubDataHandling SubParser::handleSubdata(core::Bytes id){
	return text::Parser::SubDataNone;
}
void SubParser::subdata(core::Bytes data){
}
void SubParser::end(){
}

struct String: core::Bytes {
	String() : core::Bytes(nullptr,nullptr) {}
	void operator = (const core::Bytes& b);
};
void String::operator = (const core::Bytes& b){ begin=b.begin;end=b.end; }

class Texture: public SubParser {
public:
	String id;
	String path;
	bool mipmapping;
	rendering::Texture2D result;

	Texture(): mipmapping(false) {}
	void set(core::Bytes id);
	void end();
};

//A mesh from the text bundle importer
class Mesh: public SubParser {
	String id;
	String path;
	String materialId;

	void set(core::Bytes id);
	void end();
};

void Mesh::set(core::Bytes id){
	if(equals(id,"id"))	this->id = parser->string();
	else if(equals(id,"path")) path = parser->string();
	else if(equals(id,"material")) materialId = parser->string();
	else if(equals(id,"scale")) auto scalar = parser->number();
}
void Mesh::end(){

	class Reader: public intermediate::mesh::Reader {
	public:
		Parser* parser;
		internal_::Service* service;
		internal_::ResourceBundle* bundle;
		Mesh* self;
		::data::Mesh* destMesh;
		uint32 animationId;
		uint32 materialId;

		enum { kMaxMaterials = 128 };
		
		::data::Material* materials[kMaxMaterials];

		Reader() : animationId(0),materialId(0) { for(size_t i =0;i < kMaxMaterials;++i) materials[i] = nullptr; }

		void processMesh(const intermediate::Mesh* submeshes,const ::data::Mesh& mesh,uint32 vertexFormat) {
			
			size_t arrayExtras = mesh.submeshCount() < 2? 0 : sizeof(::data::SubMesh*)*mesh.submeshCount();
			auto destMesh = service->allocateObject<::data::Mesh>(arrayExtras + sizeof(SubMesh)*mesh.submeshCount() +
				mesh.skeletonNodeCount()*(sizeof(::data::Mesh::SkeletonJointId)+sizeof(::data::Transformation3D)) + 
				mesh.skeletonNodeCount()? alignof(::data::Transformation3D) : 0);
			uint8* ptr = (uint8*)(destMesh+1);
			::data::Mesh::SkeletonJointId* destParentIds = nullptr;
			::data::Transformation3D* destLocalTransformations = nullptr;
			if(mesh.skeletonNodeCount()){
				destParentIds = (::data::Mesh::SkeletonJointId*) ptr;
				memcpy(destParentIds,mesh.skeletonHierarchy_,sizeof(::data::Mesh::SkeletonJointId)*mesh.skeletonNodeCount());
				ptr+=sizeof(::data::Mesh::SkeletonJointId)*mesh.skeletonNodeCount();
				ptr = (uint8*)core::memory::align_forward(ptr,alignof(::data::Transformation3D));
				destLocalTransformations = (::data::Transformation3D*)ptr;
				memcpy(destLocalTransformations,mesh.skeletonLocalTransforms_,sizeof(::data::Transformation3D)*mesh.skeletonNodeCount());
				ptr+=sizeof(::data::Transformation3D)*mesh.skeletonNodeCount();
			}
			auto destArray = (::data::SubMesh**)ptr;ptr+=arrayExtras;
			auto destSubmeshes = (::data::SubMesh*) ptr;
			
			assert((uintptr_t(destSubmeshes) % alignof(::data::SubMesh)) == 0);
			
			for(size_t i = 0;i< mesh.submeshCount();++i){
				auto rendering = services::rendering();
				auto vertexBuffer = rendering->create(rendering::Buffer::Vertex,false,submeshes[i].vertices.length(),submeshes[i].vertices.begin);
				auto indexBuffer  = rendering->create(rendering::Buffer::Index,false,submeshes[i].indices.length(),submeshes[i].indices.begin);
				rendering::VertexDescriptor vd;
				if(vertexFormat & Options::VertexWeights){
					//position,normal,texcoord,bone weights
					static core::TypeDescriptor posNTW[4] = { { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TFloat,2 } , { core::TypeDescriptor::TFloat,4 } };
					vd.fields = posNTW;
					vd.count = 4;
				} else {
					vd = rendering::VertexDescriptor::positionAs3Float_normalAs3Float_texcoordAs2Float();
				}
				auto m = rendering->create(vertexBuffer,indexBuffer,vd);
				auto count = submeshes[i].indices.length()/submeshes[i].indexSize;

				new(destSubmeshes + i) ::data::SubMesh(m,0,count,submeshes[i].indexSize,rendering::topology::Triangle);
				destSubmeshes[i].skeletonJoints_ = submeshes[i].joints;

				auto mat = submeshes[i].materialIndex;
				if(mat >= materialId) destSubmeshes[i].material_ = nullptr;
				else destSubmeshes[i].material_ = materials[mat];
			}
			
			if(mesh.submeshCount() < 2){
				new(destMesh) ::data::Mesh(destSubmeshes);
			} else {
				for(size_t i = 0;i< mesh.submeshCount();++i) destArray[i] = destSubmeshes + i;
				new(destMesh) ::data::Mesh(destArray,mesh.submeshCount());
			}
			destMesh->boneCount_ = mesh.boneCount_;
			destMesh->skeletonHierarchy_ = destParentIds;
			destMesh->skeletonLocalTransforms_ = destLocalTransformations;
			service->mapPointerToID(bundle,destMesh,self->id);
		}
		void Reader::processMaterial(const char* name,const char** textures,size_t textureCount) {
			core::bufferStringStream::Formatter fmt;
			if(!name || !strlen(name)) {
				using namespace core::bufferStringStream;
				name = asCString(fmt.allocator<<self->id<<".material."<<materialId);
			}

			auto destMaterial = service->allocateObject<::data::Material>();
			rendering::Texture2D ts[::data::Material::kMaxTextures];
			for(size_t i = 0;i<textureCount;++i){
				Texture texture;
				texture.parser = parser;
				texture.id = core::Bytes(nullptr,nullptr);
				texture.mipmapping = true;
				texture.path = core::Bytes((void*)textures[i],strlen(textures[i]));
				texture.end();
				ts[i] = texture.result;
			}
			materials[materialId] = new(destMaterial) ::data::Material(ts,textureCount,nullptr,0);
			materialId++;
		}
		void processSkeletalAnimation(const char* name,const animation::Animation& animation) {

			core::bufferStringStream::Formatter fmt;
			if(!name || !strlen(name)) {
				using namespace core::bufferStringStream;
				name = asCString(fmt.allocator<<self->id<<".animation."<<animationId);
			}
			animationId++;

			size_t memory = sizeof(animation::Track) * animation.trackCount + alignof(vec4f);
			for(uint32 i  = 0;i<animation.trackCount;++i){
				memory+=animation.tracks[i].positionKeyCount * sizeof(animation::PositionKey) +
					animation.tracks[i].rotationKeyCount * sizeof(animation::RotationKey) +
					animation.tracks[i].scalingKeyCount * sizeof(animation::ScalingKey);
			}
			auto destAnimation = service->allocateObject<animation::Animation>(memory);
			destAnimation->length = animation.length;
			destAnimation->frequency = animation.frequency;
			destAnimation->trackCount = animation.trackCount;
			destAnimation->tracks = (animation::Track*)(destAnimation+1);
			uint8* ptr = (uint8*)(destAnimation->tracks+animation.trackCount);
			ptr = (uint8*)core::memory::align_forward(ptr,alignof(vec4f));
			for(uint32 i  = 0;i<animation.trackCount;++i){
				core::bufferStringStream::Formatter fmt;
				core::bufferStringStream::printf(fmt.allocator,"Track[%d] = bone %d",i,animation.tracks[i].nodeId);
				services::logging()->trace(core::bufferStringStream::asCString(fmt.allocator));

				destAnimation->tracks[i].nodeId = animation.tracks[i].nodeId;
				destAnimation->tracks[i].positionKeyCount = animation.tracks[i].positionKeyCount;
				destAnimation->tracks[i].positionKeys = (animation::PositionKey*) ptr;
				memcpy(ptr,animation.tracks[i].positionKeys,sizeof(animation::PositionKey) * animation.tracks[i].positionKeyCount);
				ptr+=sizeof(animation::PositionKey) * animation.tracks[i].positionKeyCount;
				
				destAnimation->tracks[i].rotationKeyCount = animation.tracks[i].rotationKeyCount;
				destAnimation->tracks[i].rotationKeys = (animation::RotationKey*) ptr;
				memcpy(ptr,animation.tracks[i].rotationKeys,sizeof(animation::RotationKey) * animation.tracks[i].rotationKeyCount);
				ptr+=sizeof(animation::RotationKey) * animation.tracks[i].rotationKeyCount;
				
				destAnimation->tracks[i].scalingKeyCount = animation.tracks[i].scalingKeyCount;
				destAnimation->tracks[i].scalingKeys = (animation::ScalingKey*) ptr;
				memcpy(ptr,animation.tracks[i].scalingKeys,sizeof(animation::ScalingKey) * animation.tracks[i].scalingKeyCount);
				ptr+=sizeof(animation::ScalingKey) * animation.tracks[i].scalingKeyCount;
			}
			
			service->mapPointerToID(bundle,destAnimation,core::Bytes((void*)name,strlen(name)));
		}
	};
	Reader reader;reader.parser = parser;reader.service = parser->service;reader.bundle = parser->bundle;reader.self = this;
	auto p = parser->makePath(path);
	parser->pushPath(path);
	reader.load(core::memory::globalAllocator(),p);
	parser->popPath();
}


class Pipeline: public SubParser {
	uint32 embeddedShaderType;

	String id;
	String shaderPaths  [rendering::Shader::MaxTypes];
	String shaderSources[rendering::Shader::MaxTypes];
	enum { kMaxVertexAttributes = 16 };
	uint32 vertexAttributeCount;
	String vertexAttributes[kMaxVertexAttributes ];

	void set(core::Bytes id);
	text::Parser::SubDataHandling handleSubdata(core::Bytes id);
	void subdata(core::Bytes data);
	void end();
};
void Pipeline::set(core::Bytes id){
	if(equals(id,"id")) this->id = parser->string();
	else if(equals(id,"vertexShader")) shaderPaths[rendering::Shader::Vertex] = parser->string();
	else if(equals(id,"pixelShader")) shaderPaths[rendering::Shader::Pixel] = parser->string();
	else if(equals(id,"geometryShader")) shaderPaths[rendering::Shader::Geometry] = parser->string();
	else if(equals(id,"glsl.vertexAttributes")) vertexAttributeCount = parser->strings(vertexAttributes,Pipeline::kMaxVertexAttributes);
}
text::Parser::SubDataHandling Pipeline::handleSubdata(core::Bytes id) {
	embeddedShaderType = rendering::Shader::MaxTypes;
	if(equals(id,"vertexShader")){
		embeddedShaderType = rendering::Shader::Vertex;
	} else if(equals(id,"pixelShader")){
		embeddedShaderType = rendering::Shader::Pixel;
	} else if(equals(id,"geometryShader")){
		embeddedShaderType = rendering::Shader::Geometry;
	}
	return embeddedShaderType != rendering::Shader::MaxTypes? text::Parser::SubDataCustomBlock : text::Parser::SubDataNone;
}
void Pipeline::subdata(core::Bytes data) {
	shaderSources[embeddedShaderType] = data;
}
void Pipeline::end(){

	auto rendering = services::rendering();

	shader::Preprocessor preprocessor(core::memory::globalAllocator());

	//Check shader paths / sources
	rendering::Shader shaders[rendering::Shader::MaxTypes];
	uint32 shaderCount = 0;
	for(uint32 i = 0;i<rendering::Shader::MaxTypes;++i){
		if(!shaderPaths[i].empty()){
			io::Data data(parser->makePath(shaderPaths[i]));
			auto src = preprocessor.preprocess(core::Bytes(data.begin,data.size));
			shaders[shaderCount] = rendering->create(rendering::Shader::Type(i),(char*)src.begin,src.length());
			parser->service->registerShader(shaders[shaderCount]);
			++shaderCount;
		}
	}
	for(uint32 i = 0;i<rendering::Shader::MaxTypes;++i){
		if(!shaderSources[i].empty()){
			if(!shaderPaths[i].empty()){
				//Error
				continue;
			}
			auto src = preprocessor.preprocess(shaderSources[i]);
			shaders[shaderCount] = rendering->create(rendering::Shader::Type(i),(char*)src.begin,src.length());
			parser->service->registerShader(shaders[shaderCount]);
			++shaderCount;
		}
	}

#ifdef ARPHEG_RENDERING_GL
	rendering::Pipeline::GLSLLayout glslLayout;
	char attributeBufferStorage[2048];
	core::BufferAllocator attributeBuffer(core::Bytes(attributeBufferStorage,sizeof(attributeBufferStorage)),core::BufferAllocator::GrowOnOverflow);
	uint32 attributeBufferOffset = 0;
	if(vertexAttributeCount){
		glslLayout.vertexAttributes = (const char**) attributeBuffer.allocate(vertexAttributeCount * sizeof(char*),alignof(char*));
		glslLayout.vertexAttributeCount = vertexAttributeCount;
		for(uint32 i = 0;i< vertexAttributeCount;++i){
			auto dest = (char*)attributeBuffer.allocate(vertexAttributes[i].length()+1,0);
			memcpy(dest,vertexAttributes[i].begin,vertexAttributes[i].length());
			dest[vertexAttributes[i].length()] = '\0';
			glslLayout.vertexAttributes[i] = dest;
		}
	}
	auto pipeline = rendering->create(shaders,shaderCount,glslLayout);
#else
	auto pipeline = rendering->create(shaders,shaderCount);
#endif

	auto obj = parser->service->newPipeline();
	new(obj) ::data::Pipeline(pipeline);
	parser->service->mapPointerToID(parser->bundle,obj,id);
	parser->service->registerPipeline(pipeline);
}

//

class TextureArray: public SubParser {
public:
	String id;
	enum { kMaxTextures = 64 };
	uint32 count;
	String paths[kMaxTextures];
	bool mipmapping;

	TextureArray(): mipmapping(false),count(0) {}
	void set(core::Bytes id);
	void end();
};

void Texture::set(core::Bytes id){
	if(equals(id,"id")) this->id = parser->string();
	else if(equals(id,"path")) path = parser->string();
	else if(equals(id,"mipmapping")) mipmapping = parser->boolean();
}
void Texture::end() {
	auto rendering = services::rendering();
	class Reader: public image::Reader {
	public:
		rendering::Texture2D result;
		void processData(const rendering::texture::Descriptor2D& format,const void* data,size_t dataSize,uint32 stride) {
			result = services::rendering()->create(format,data);	
		}
	};
	Reader reader;
	reader.load(parser->makePath(path));
	if(mipmapping) rendering->generateMipmaps(reader.result);
	
	auto obj = parser->service->allocateObject<rendering::Texture2D>();
	*obj = reader.result;
	result = reader.result;
	if(!id.empty()) parser->service->mapPointerToID(parser->bundle,obj,id);
	parser->service->renderingTexture2D(reader.result);
}
void TextureArray::set(core::Bytes id){
	if(equals(id,"id")) this->id = parser->string();
	else if(equals(id,"path")) count = parser->strings(paths,kMaxTextures);
	else if(equals(id,"mipmapping")) mipmapping = parser->boolean();
}
void TextureArray::end(){
	core::BufferAllocator buffer(1024*1024*count,core::memory::globalAllocator(),core::BufferAllocator::GrowOnOverflow);

	class Reader: public image::Reader {
	public:
		core::BufferAllocator& dest;
		rendering::texture::Descriptor2D format;
		Reader(core::BufferAllocator& destination) : dest(destination) {
		}
		void processData(const rendering::texture::Descriptor2D& format,const void* data,size_t dataSize,uint32 stride) {
			this->format = format;
			auto dst = dest.allocate(dataSize,0);
			memcpy(dst,data,dataSize);
		}
	};
	Reader reader(buffer);
	for(size_t i =0;i< count;++i){
		reader.load(parser->makePath(paths[i]));
	}

	auto result  = services::rendering()->create(reader.format,count,buffer.bufferBase());
	if(mipmapping) services::rendering()->generateMipmaps(result);
	
	auto obj = parser->service->allocateObject<rendering::Texture2DArray>();
	*obj = result;
	parser->service->mapPointerToID(parser->bundle,obj,id);
	//parser->service->renderingTexture2DArray(result); TODO
}

class Font: public SubParser {
public:
	String id, path;
	void set(core::Bytes field);
	void end();
};
void Font::set(core::Bytes field){
	if(equals(field,"id")) id = parser->string();
	else if(equals(field,"path")) path = parser->string();
}
void Font::end(){
	class Reader: public font::Reader {
	public:
		Parser* parser;
		void processTextureRequest(core::Bytes path){
			class Reader: public image::Reader {
			public:
				rendering::Texture2D result;
				void processData(const rendering::texture::Descriptor2D& format,const void* data,size_t dataSize,uint32 stride) {
					result = services::rendering()->create(format,data);	
				}
			};
			Reader reader;
			reader.load(parser->makePath(path));
			
			//Register texture for deletion
			parser->service->renderingTexture2D(reader.result);

			result.pages_[result.pageCount_] = reader.result;
			result.pageCount_ ++;
		}
	};
	Reader reader;reader.parser = parser;

	io::Data file(parser->makePath(path));
	parser->pushPath(path);
	reader.load(core::Bytes(file.begin,file.size));
	parser->popPath();

	auto font = new(parser->service->allocateObject< ::data::Font>()) ::data::Font(reader.result);
#ifdef ARPHEG_RENDERING_GL
	//font->flipYTexcoord();
#endif
	parser->service->mapPointerToID(parser->bundle,font,id);
}

class Sprite: public SubParser {
public:
	String id, texture;
	vec4f frame;
	vec2f size;

	Sprite();
	void set(core::Bytes field);
	void end();
};
Sprite::Sprite() : frame(0,0,1,1),size(1,1) {}
void Sprite::set(core::Bytes field){
	if(equals(field,"id")) id = parser->string();
	else if(equals(field,"texture")) texture = parser->string();
	else if(equals(field,"frame")) frame = parser->vec4();
	else if(equals(field,"size")) size = parser->vec2();
}
void Sprite::end(){
	uint32 frameCount = 1;
	auto sprite = new(parser->service->allocateObject< ::data::Sprite>(frameCount*sizeof(::data::Sprite::Frame))) ::data::Sprite(vec2i(int(size.x),int(size.y)));	
	sprite->frameCount_ = frameCount;
	sprite->frames_     = (::data::Sprite::Frame*)(sprite+1);
	sprite->texture_    = *(rendering::Texture2D*)parser->service->getResourceFromID(parser->bundle,texture);
	sprite->frames_[0] = ::data::Sprite::Frame(&frame.x);

	parser->service->mapPointerToID(parser->bundle,sprite,id);
}


struct SubparserMapper {
	const char* key;
	SubParser* (*factory) (void*);
};
Parser::Parser() : 
	subparserBuffer_(core::Bytes(subparserBufferStorage_,sizeof(subparserBufferStorage_))),
	pathBuffer(core::Bytes(pathBufferStorage,sizeof(pathBufferStorage))),
	pathBufferStack(core::Bytes(pathBufferStackStorage,sizeof(pathBufferStackStorage))),
	currentPath_(nullptr,nullptr),bundlePath_(nullptr,nullptr) 
{
		registerSubdata<Mesh>        ("mesh");
		registerSubdata<Pipeline>    ("pipeline");
		registerSubdata<Texture>     ("texture");
		registerSubdata<TextureArray>("textureArray");
		registerSubdata<Font>("font");
		registerSubdata<Sprite>("sprite");
}

void Parser::parse(core::Bytes bundlePath,core::Bytes bytes,data::internal_::Service* service,data::internal_::ResourceBundle* bundle) {
	this->service = service;
	this->bundle  = bundle;
	
	bundlePath_ = currentPath_ = bundlePath;
	subparser = nullptr;
	text::Parser::parse(bytes);
}
void Parser::pushPath(core::Bytes subpath){
	core::bufferStringStream::Formatter fmt;
	
	utils::path::dirname(fmt.allocator,subpath);

	auto size = pathBufferStack.size();
	utils::path::join(pathBufferStack,currentPath_,core::Bytes(fmt.allocator.bufferBase(),fmt.allocator.bufferTop()));
	currentPath_.begin = pathBufferStack.bufferBase() + size;
	currentPath_.end   = pathBufferStack.bufferTop();
}
void Parser::popPath() {
	currentPath_ = bundlePath_;
	pathBufferStack.reset();
}
const char* Parser::makePath(core::Bytes subpath) {
	pathBuffer.reset();
	utils::path::join(pathBuffer,currentPath_,subpath);
	return core::bufferStringStream::asCString(pathBuffer);
}
void Parser::registerSubdata(const char* id,size_t size,SubParser* (* factory)(void* )) {
	if(size >= sizeof(dataBuffer)){
		assertRelease(false && "data::bundle::Parser data buffer is too small!");
		return;
	}
	auto s = core::bufferArray::allocate<SubparserMapper>(subparserBuffer_);
	s->key = id;
	s->factory = factory;
}

SubParser* Parser::identifySubdata(core::Bytes id) {
	auto i = core::bufferArray::begin<SubparserMapper>(subparserBuffer_);
	auto end = core::bufferArray::end<SubparserMapper>(subparserBuffer_);
	for(;i<end;++i){
		if(compare(id,i->key)){
			return i->factory((void*)dataBuffer);
		}
	}
	return nullptr;
}
Parser::SubDataHandling Parser::handleSubdata(core::Bytes id) {
	if(subparser) return subparser->handleSubdata(id);
	data = nullptr;
	subparser = identifySubdata(id);
	if(subparser){
		subparser->parser = this;
		return SubDataObject;
	}
	return SubDataNone;
}
void Parser::endSubdata(){
	if(subparser){
		subparser->end();
		subparser= nullptr;
	}
}
void Parser::subdata(core::Bytes data) {
	if(subparser) subparser->subdata(data);
}
void Parser::set(core::Bytes bytes){
	if(subparser) subparser->set(bytes);
}

} } }