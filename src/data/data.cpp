#include <string.h>

#include "../services.h"
#include "../core/assert.h"
#include "../core/io.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../core/bufferStack.h"
#include "../core/bufferStringStream.h"
#include "data.h"
#include "image/reader.h"

#include "internal/internal.h"
#include "intermediate/bundle/parser.h"

#include "utils/path.h"

namespace data {

namespace internal_ {

static void releaseBuffer(rendering::Service* service,void* data){
	service->release(*((rendering::Buffer*)data));
}
static void releaseMesh(rendering::Service* service,void* data){
	service->release(*((rendering::Mesh*)data));
}
static void releaseTexture2D(rendering::Service* service,void* data){
	service->release(*((rendering::Texture2D*)data));
}
static void releaseShader(rendering::Service* service,void* data){
	service->release(*((rendering::Shader*)data));
}
static void releasePipeline(rendering::Service* service,void* data){
	service->release(*((rendering::Pipeline*)data));
}
static void releaseSampler(rendering::Service* service,void* data){
	service->release(*((rendering::Sampler*)data));
}

void ResourceName::operator =(const char* str) {
	strncpy(string,str,kMaxLength);
	string[kMaxLength-1] = '\0';//Null terminate
}
void ResourceName::operator =(core::Bytes str) {
	auto len = std::min(size_t(kMaxLength-1),str.length());
	memcpy(string,str.begin,len);
	string[len] = '\0';
}


//An intermediate pointer mapper, slow, not to be used in final builds.
struct IDPointerMapper {
	struct Obj {
		core::Bytes key;
		void* value;
	};
	core::BufferAllocator objects;
	core::Allocator* stringStorage;
	
	IDPointerMapper(core::Allocator* allocator);
	
	void insert(core::Bytes key,void* value);
	void* get(core::Bytes key);
};
IDPointerMapper::IDPointerMapper(core::Allocator* allocator) :
	objects(sizeof(IDPointerMapper) * 128,allocator,core::BufferAllocator::GrowOnOverflow)
{ stringStorage = allocator; }
void IDPointerMapper::insert(core::Bytes key,void* value){
	//copy string
	auto dest = (uint8*)stringStorage->allocate(key.length()+1);
	memcpy(dest,key.begin,key.length());
	dest[key.length()] = '\0';
	//
	auto obj = core::bufferArray::allocate<Obj>(objects);
	obj->key = core::Bytes(dest,key.length());
	obj->value = value;
}
void* IDPointerMapper::get(core::Bytes key){
	auto i = core::bufferArray::begin<Obj>(objects);
	auto end = core::bufferArray::end<Obj>(objects);
	for(;i<end;++i){
		if(i->key.length() == key.length() && memcmp(i->key.begin,key.begin,key.length()) == 0) 
			return i->value;
	}
	return nullptr;
}

ObjectStack::ObjectStack(size_t size) : core::BufferAllocator(size,nullptr,core::BufferAllocator::GrowOnOverflow) {

}

Service::Service() {
	currentBundle = nullptr;
	newBundle("<Root data bundle>");
}
Service::~Service() {
	releaseBundles();
	currentBundle = nullptr;
}
ResourceBundle* Service::newBundle(const char* name){
	//Create a new bundle and save resource stack context for it.
	auto bundle = bundles.createNew();
	bundle->name = name;
	bundle->renderObjects = renderObjects.stackSection();
	bundle->submeshes = submeshes.stackSection();
	bundle->pipelines = pipelines.stackSection();
	bundle->objects   = objects.stackSection();
	//
	assertRelease(sizeof(bundle->mappingStorage) >= sizeof(IDPointerMapper));
	new(bundle->mappingStorage) IDPointerMapper(core::memory::globalAllocator());
	//Use the new bundle for resource id lookup.
	currentBundle = bundle;
	return bundle;
}
void Service::releaseBundles() {
	while(!core::bufferStack::isEmpty<ResourceBundle>(bundles))
		releaseTopBundle();
}
void Service::releaseTopBundle() {
	//Make sure to clear the currentDirectory
	currentBundle = nullptr;
	
	assert(!core::bufferStack::isEmpty<ResourceBundle>(bundles));
	auto bundle = core::bufferStack::pop<ResourceBundle>(bundles);
	//Release rendering objects
	struct F { rendering::Service* service; inline void operator ()(RenderResource& r)  { r.func(service,&r.obj); } };
	F f;f.service = services::rendering();
	renderObjects.foreach(f,bundle.renderObjects);
	renderObjects.release(bundle.renderObjects);

	//Release resources
	submeshes.release(bundle.submeshes);
	pipelines.release(bundle.pipelines);
	objects.release(bundle.objects);

	//Trace
	using namespace core::bufferStringStream;
	Formatter fmt;
	printf(fmt.allocator,"Released data bundle '%s'",bundle.name.string);
	services::logging()->trace(asCString(fmt.allocator));
}
BundleID Service::loadBundle(const char* filename,const char* id) {
	io::Data file(filename);
	auto bundle = newBundle(filename);
	bundle->name = id;
	
	char storage[2048];
	core::BufferAllocator buffer(core::Bytes(storage,sizeof(storage)));
	utils::path::dirname(buffer,core::Bytes((void*)filename,strlen(filename)));
	loadTextBundle(core::bufferStringStream::asCString(buffer),bundle,core::Bytes(file.begin,file.size));
	return BundleID(core::bufferArray::length<ResourceBundle>(bundles) - 1);
}
BundleID Service::getBundle(const char* id,bool optional) {
	using namespace core::bufferArray;
	for(size_t i = 0;i<length<ResourceBundle>(bundles);++i){
		if( !strcmp(nth<ResourceBundle>(bundles,i).name.string,id) ){
			return BundleID(i);
		}
	}
	if(!optional) assertRelease(false && "Can't find a given bundle");
	return 0;
}
void Service::loadTextBundle(const char* path,ResourceBundle* bundle,core::Bytes bytes){
	intermediate::bundle::Parser parser;
	parser.parse(core::Bytes((void*)path,strlen(path)),bytes,this,bundle);
}
void Service::endBundle() {
}

void* Service::getResourceFromID(const char* id) {
	if(!currentBundle){
		assertRelease(false && "Can't find a resource without selected bundle");
		return nullptr;
	}
	auto mapper = (IDPointerMapper*)currentBundle->mappingStorage;
	auto p = mapper->get(core::Bytes((void*)id,strlen(id)));
	assertRelease(p);
	return p;
}
void* Service::getResourceFromID(BundleID bundle,const char* id,bool optional) {
	assert(bundle < core::bufferArray::length<ResourceBundle>(bundles));
	auto mapper = (IDPointerMapper*)core::bufferArray::nth<ResourceBundle>(bundles,bundle).mappingStorage;
	auto p = mapper->get(core::Bytes((void*)id,strlen(id)));
	if(!optional) assertRelease(p);
	return p;
}
void* Service::getResourceFromID(ResourceBundle* bundle,core::Bytes id) {
	auto mapper = (IDPointerMapper*)bundle->mappingStorage;
	auto p = mapper->get(id);
	return p;
}
void Service::mapPointerToID(ResourceBundle* bundle,void* ptr,core::Bytes id) {
	auto mapper = (IDPointerMapper*)bundle->mappingStorage;
	mapper->insert(id,ptr);
}
SubMesh* Service::newSubMesh() {
	return submeshes.createNew();
}
Pipeline* Service::newPipeline() {
	return pipelines.createNew();
}

#define REGISTER_RENDEROBJ(Tag,v) \
	auto obj = renderObjects.createNew(); \
	obj->func = & release##Tag;  \
	obj->obj.v = v; services::logging()->trace("Registered rendering " #v);

void Service::renderingBuffer(rendering::Buffer buffer){
	REGISTER_RENDEROBJ(Buffer,buffer);
}
void Service::renderingMesh(const rendering::Mesh& mesh){
	REGISTER_RENDEROBJ(Mesh,mesh);
}
void Service::renderingTexture2D(rendering::Texture2D texture2D){
	REGISTER_RENDEROBJ(Texture2D,texture2D);
}
void Service::registerShader(rendering::Shader shader){
	REGISTER_RENDEROBJ(Shader,shader);
}
void Service::registerPipeline(rendering::Pipeline pipeline){
	REGISTER_RENDEROBJ(Pipeline,pipeline);
}

#undef REGISTER_RENDEROBJ

//Importing

} // namespace internal_

inline internal_::Service* Service::impl() const {
	return pimpl;
}
Service::Service(core::Allocator* allocator){
	pimpl = ALLOCATOR_NEW(allocator,internal_::Service);
}
Service::~Service(){
	impl()->~Service();
}

Pipeline::Pipeline(rendering::Pipeline pipeline) {
	pipeline_ = pipeline;
}
Sprite::Sprite(vec2i size) {
	assert(size.x <= std::numeric_limits<uint16>::max() && size.y <= std::numeric_limits<uint16>::max());
	size_ = uint32(size.x) | (uint32(size.y)<<16);
}
Sprite::Frame::Frame(float* texcoords) {
	textureCoords[0] = normalizedUint16::make(texcoords[0]);
	textureCoords[1] = normalizedUint16::make(texcoords[1]);
	textureCoords[2] = normalizedUint16::make(texcoords[2]);
	textureCoords[3] = normalizedUint16::make(texcoords[3]);
}
Material::Material(const rendering::Texture2D* textures,size_t textureCount,const void* constants,size_t constantsSize) {
	assert(textureCount <= kMaxTextures);
	data_ = uint32(textureCount);
	for(size_t i =0;i< textureCount;++i) textures_[i] = textures[i];
	if(constantsSize < kConstantsSize) memset(parameterStorage_,0,sizeof(parameterStorage_));
	if(constantsSize) memcpy(parameterStorage_,constants,constantsSize);
}
SubMesh::SubMesh(const rendering::Mesh& mesh,uint32 offset,uint32 count,uint32 indexSize,rendering::topology::Primitive mode){
	mesh_ = mesh;
	data_ = count&kCountMask | ((indexSize&kIndexSizeMask)<<kIndexOffset) | ((uint32(mode)&kIndexSizeMask)<<kPrimOffset);
	primitiveOffset_ = offset;
}
Mesh::Mesh(SubMesh* singleSubMesh) {
	submeshCount_ = 1;
	boneCount_ = 0;
	submeshes_.oneSubmesh = singleSubMesh;
	skeletonHierarchy_ = nullptr;
	skeletonLocalTransforms_ = nullptr;
}
Mesh::Mesh(SubMesh** submeshes,size_t count) {
	submeshCount_ = count;
	boneCount_ = 0;
	submeshes_.manySubMeshes = submeshes;
	skeletonHierarchy_ = nullptr;
	skeletonLocalTransforms_ = nullptr;
}

BundleID Service::loadBundle(const char* filename) {
	return impl()->loadBundle(filename,"");
}
BundleID Service::bundle(ID id,bool optional) {
	return impl()->getBundle(id,optional);
}
rendering::Texture2D      Service::texture2D(ID id){
	return * (rendering::Texture2D*) impl()->getResourceFromID(id);
}
rendering::Texture2DArray Service::texture2DArray(ID id){
	return * (rendering::Texture2DArray*) impl()->getResourceFromID(id);
}
rendering::Sampler Service::sampler(ID id){
	return * (rendering::Sampler* ) impl()->getResourceFromID(id);
}
Pipeline* Service::pipeline(ID id) {
	return (Pipeline*) impl()->getResourceFromID(id);
}
SubMesh*  Service::submesh(ID id) {
	return (SubMesh*) impl()->getResourceFromID(id);
}
Mesh* Service::mesh(ID id) {
	return (Mesh*) impl()->getResourceFromID(id);
}
animation::Animation* Service::animation(ID id) {
	return (animation::Animation*) impl()->getResourceFromID(id);
}
Font* Service::font(ID id) {
	return (Font*) impl()->getResourceFromID(id);
}
Sprite* Service::sprite(ID id) {
	return (Sprite*) impl()->getResourceFromID(id);
}

rendering::Texture2D  Service::texture2D(BundleID bundle,ID id,bool optional){
	return * (rendering::Texture2D*) impl()->getResourceFromID(bundle,id,optional);
}
rendering::Texture2DArray Service::texture2DArray(BundleID bundle,ID id,bool optional){
	return * (rendering::Texture2DArray*) impl()->getResourceFromID(bundle,id,optional);
}
rendering::Sampler Service::sampler(BundleID bundle,ID id,bool optional){
	return * (rendering::Sampler*) impl()->getResourceFromID(bundle,id,optional);
}
Pipeline* Service::pipeline(BundleID bundle,ID id,bool optional){
	return (Pipeline*) impl()->getResourceFromID(bundle,id,optional);
}
SubMesh*  Service::submesh(BundleID bundle,ID id,bool optional){
	return (SubMesh*) impl()->getResourceFromID(bundle,id,optional);
}
Mesh*     Service::mesh(BundleID bundle,ID id,bool optional){
	return (Mesh*) impl()->getResourceFromID(bundle,id,optional);
}
animation::Animation* Service::animation(BundleID bundle,ID id,bool optional){
	return (animation::Animation*) impl()->getResourceFromID(bundle,id,optional);
}
Font*     Service::font(BundleID bundle,ID id,bool optional){
	return (Font*) impl()->getResourceFromID(bundle,id,optional);
}
Sprite*   Service::sprite(BundleID bundle,ID id,bool optional){
	return (Sprite*) impl()->getResourceFromID(bundle,id,optional);
}

//Direct loading of intermediate resources from the filesystem
rendering::Texture2D Service::loadTexture(const char* name){
	class Reader: public image::Reader {
	public:
		rendering::Texture2D result;
		void processData(const rendering::texture::Descriptor2D& format,const void* data,size_t dataSize,uint32 stride) {
			result = services::rendering()->create(format,data);	
		}
	};
	Reader reader;
	reader.load(name);
	return reader.result;
}

}