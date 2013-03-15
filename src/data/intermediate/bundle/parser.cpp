#include <string.h>
#include "../../../core/memory.h"
#include "../../../core/bufferStringStream.h"
#include "../../../core/text.h"
#include "../../../core/io.h"
#include "../../../core/allocatorNew.h"
#include "../../../core/assert.h"
#include "../../../services.h"

#include "../../internal/internal.h"

#include "../../mesh/reader.h"
#include "../../image/reader.h"
#include "../../shader/preprocess.h"
#include "../../utils/path.h"
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
		internal_::Service* service;
		internal_::ResourceBundle* bundle;
		Mesh* self;

		bool processMesh(const intermediate::Mesh& mesh,const intermediate::Material* material){
			auto rendering = services::rendering();
			auto vertexBuffer = rendering->create(rendering::Buffer::Vertex,false,mesh.vertices.length(),mesh.vertices.begin);
			auto indexBuffer  = rendering->create(rendering::Buffer::Index,false,mesh.indices.length(),mesh.indices.begin);
			auto m = rendering->create(vertexBuffer,indexBuffer,rendering::VertexDescriptor::positionAs3Float_normalAs3Float_texcoordAs2Float());
			auto count = mesh.indices.length()/mesh.indexSize;
				
			//Create the new submesh
			auto submesh = new(service->newSubMesh()) ::data::SubMesh(m,0,count,sizeof(uint16),rendering::topology::Triangle);
			service->mapPointerToID(bundle,submesh,self->id);

			//Register imported data
			service->renderingBuffer(vertexBuffer);
			service->renderingBuffer(indexBuffer);
			service->renderingMesh(m);
			
			//Free the storage memory
			return true;
		}
	};
	Reader reader;reader.service = parser->service;reader.bundle = parser->bundle;reader.self = this;
	reader.load(core::memory::globalAllocator(),parser->makePath(path));
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
class Material: public SubParser {
public:
	String id;
	/*union Attribute {
		s
	};
	struct Texture {
		enum { Diffuse = 1, Normal = 2,Specular = 4, Displacement = 8, Opacity = 0x10, Light = 0x20 };
		uint32 type;
		String path;
	};
	uint32 textureCount;
	enum { kMaxTextures = 8 };
	Texture textures[kMaxTextures];
	vec3f diffuse, specular, ambient, emmisive;
	float shininess, roughness, opacity;*/
	void set(core::Bytes id);
};
void Material::set(core::Bytes id){
	if(equals(id,"id"))
		this->id = parser->string();
}

class Texture: public SubParser {
public:
	String id;
	String path;
	bool mipmapping;

	Texture(): mipmapping(false) {}
	void set(core::Bytes id);
	void end();
};
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
	parser->service->mapPointerToID(parser->bundle,obj,id);
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
	font->flipYTexcoord();
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
	auto sprite = new(parser->service->allocateObject< ::data::Sprite>(frameCount*sizeof(::data::Sprite::Frame))) ::data::Sprite();
	sprite->frameCount_ = frameCount;
	sprite->frames_     = (::data::Sprite::Frame*)(sprite+1);
	sprite->texture_    = *(rendering::Texture2D*)parser->service->getResourceFromID(parser->bundle,texture);
	sprite->frames_[0].texcoordMin = vec2f(frame.x,frame.y);
	sprite->frames_[0].texcoordMax = vec2f(frame.z,frame.w);
	sprite->size_ = size;
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
		registerSubdata<Material>("material");
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
	pathBuffer.reset();
	utils::path::dirname(pathBuffer,subpath);

	auto size = pathBufferStack.size();
	utils::path::join(pathBufferStack,currentPath_,core::Bytes(pathBuffer.bufferBase(),pathBuffer.bufferTop()));
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