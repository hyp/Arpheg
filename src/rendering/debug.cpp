#include <limits>
#include <string.h>
#include "../core/assert.h"
#include "../core/thread/threadlocal.h"
#include "../core/thread/thread.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../application/tasking.h"
#include "../services.h"
#include "rendering.h"
#include "debug.h"

//TODO triangles

namespace rendering {
namespace debug {

//Position + colour
static const size_t kVertexSize = sizeof(float)*3 + sizeof(uint8)*4;

#ifndef ARPEG_PLATFORM_NOTHREAD

static const size_t kThreadVertexBufferSize = 2048;
static const size_t kThreadIndexBufferSize  = 768;

THREAD_LOCAL(uint32,threadLineFrameId) = 0;
THREAD_LOCAL(core::BufferAllocator*,threadLineVertices) = nullptr;
THREAD_LOCAL(core::BufferAllocator*,threadLineIndices) = nullptr;
THREAD_LOCAL(uint32,threadTriangleFrameId) = 0;
THREAD_LOCAL(core::BufferAllocator*,threadTriangleVertices) = nullptr;
THREAD_LOCAL(core::BufferAllocator*,threadTriangleIndices) = nullptr;

struct ThreadBufferRegistration {
	bool isLines;
	core::BufferAllocator* vertices;
	core::BufferAllocator* indices;
};

void Service::getThreadLineBuffers(core::BufferAllocator*& vertices,core::BufferAllocator*& indices){
	if(threadLineFrameId == services::application()->frameID()){
		vertices = threadLineVertices;
		indices  = threadLineIndices;
		return;
	}
	threadLineFrameId = services::application()->frameID();
	auto memory = (core::BufferAllocator*) services::threadSafeFrameAllocator()->allocate(sizeof(core::BufferAllocator)*2 + kThreadVertexBufferSize+kThreadIndexBufferSize,alignof(core::BufferAllocator));
	auto datMemory = ((uint8*)memory) + sizeof(core::BufferAllocator)*2;
	vertices = threadLineVertices = new(memory) core::BufferAllocator(core::Bytes(datMemory,kThreadVertexBufferSize));
	indices = threadLineIndices  = new(memory+1) core::BufferAllocator(core::Bytes(datMemory+kThreadVertexBufferSize,kThreadIndexBufferSize));
	ThreadBufferRegistration reg = {true,vertices,indices};
	{
		core::Lock lock(*threadBufferMutex_);
		core::bufferArray::add(threadBuffers_,reg);
	}
}
void Service::getThreadTriangleBuffers(core::BufferAllocator*& vertices,core::BufferAllocator*& indices){
	if(threadTriangleFrameId == services::application()->frameID()){
		vertices = threadTriangleVertices;
		indices  = threadTriangleIndices;
		return;
	}
	threadTriangleFrameId = services::application()->frameID();
	auto memory = (core::BufferAllocator*) services::threadSafeFrameAllocator()->allocate(sizeof(core::BufferAllocator)*2 + kThreadVertexBufferSize+kThreadIndexBufferSize,alignof(core::BufferAllocator));
	auto datMemory = ((uint8*)memory) + sizeof(core::BufferAllocator)*2;
	vertices = threadTriangleVertices = new(memory) core::BufferAllocator(core::Bytes(datMemory,kThreadVertexBufferSize));
	indices = threadTriangleIndices  = new(memory+1) core::BufferAllocator(core::Bytes(datMemory+kThreadVertexBufferSize,kThreadIndexBufferSize));
	ThreadBufferRegistration reg = {false,vertices,indices};
	{
		core::Lock lock(*threadBufferMutex_);
		core::bufferArray::add(threadBuffers_,reg);
	}
}
void Service::mergeLines(core::BufferAllocator* threadLineVertices,core::BufferAllocator* threadLineIndices){
	uint32 indexOffset;
	void* vdest,*idest;
	//Locking point
	{
		core::Lock lock(*lineMutex_);
		indexOffset = lines_.size()/kVertexSize;
		vdest = lines_.allocate(threadLineVertices->size());
		idest = lineIndices_.allocate(threadLineIndices->size());
	}
	
	//Copy vertices and indices from the thread buffer to the global buffer.
	memcpy(vdest,threadLineVertices->bufferBase(),threadLineVertices->size());
	uint16* idx = (uint16*)idest;
	for(uint16* i = core::bufferArray::begin<uint16>(*threadLineIndices),*end = core::bufferArray::end<uint16>(*threadLineIndices); i< end;++i){
		*idx = *i + uint16(indexOffset);++idx;
	}

	threadLineVertices->reset();
	threadLineIndices->reset();
}
void Service::mergeTriangles(core::BufferAllocator* threadTriangleVertices,core::BufferAllocator* threadTriangleIndices){
	uint32 indexOffset;
	void* vdest,*idest;
	//Locking point
	{
		core::Lock lock(*triangleMutex_);
		indexOffset = triangles_.size()/kVertexSize;
		vdest = triangles_.allocate(threadTriangleVertices->size());
		idest = triangleIndices_.allocate(threadTriangleIndices->size());
	}
	
	//Copy vertices and indices from the thread buffer to the global buffer.
	memcpy(vdest,threadTriangleVertices->bufferBase(),threadTriangleVertices->size());
	uint16* idx = (uint16*)idest;
	for(uint16* i = core::bufferArray::begin<uint16>(*threadTriangleIndices),*end = core::bufferArray::end<uint16>(*threadTriangleIndices); i< end;++i){
		*idx = *i + uint16(indexOffset);++idx;
	}

	threadTriangleVertices->reset();
	threadTriangleIndices->reset();
}

#endif  ARPEG_PLATFORM_NOTHREAD

Service::GeometryDestination Service::allocateLines(uint32 vertexCount,uint32 indexCount,bool depthtest){
	core::BufferAllocator* vertices,*indices;
#ifndef ARPEG_PLATFORM_NOTHREAD
	getThreadLineBuffers(vertices,indices);
	if(!vertices->canAllocate(kVertexSize*vertexCount) ||
		!indices->canAllocate(sizeof(uint16)*indexCount)) 
		mergeLines(vertices,indices);
#else
	vertices = &lines_;indices = &lineIndices_;
#endif
	GeometryDestination result = { vertices->size()/kVertexSize,
		(float*)vertices->allocate(kVertexSize*vertexCount),(uint16*)indices->allocate(sizeof(uint16)*indexCount) };
	return result;
}
Service::GeometryDestination Service::allocateTriangles(uint32 vertexCount,uint32 indexCount){
	core::BufferAllocator* vertices,*indices;
#ifndef ARPEG_PLATFORM_NOTHREAD
	getThreadTriangleBuffers(vertices,indices);
	if(!vertices->canAllocate(kVertexSize*vertexCount) ||
		!indices->canAllocate(sizeof(uint16)*indexCount)) 
		mergeTriangles(vertices,indices);
#else
	vertices = &triangles_;indices = &triangleIndices_;
#endif
	GeometryDestination result = { vertices->size()/kVertexSize,
		(float*)vertices->allocate(kVertexSize*vertexCount),(uint16*)indices->allocate(sizeof(uint16)*indexCount) };
	return result;
}



void Service::viewProjectionMatrix(const mat44f& matrix) {
	viewProjection_ = matrix;
}
void Service::line(vec3f a,vec3f b,const vec4f& colour,bool depthtest){
	line(mat44f::identity(),a,b,colour,depthtest);
}
void Service::lines(const mat44f& matrix,vec3f* points,uint32 count,const vec4f& colour,bool depthtest) {
	for(uint32 i = 0;i<count;i++){
		line(matrix,points[i*2],points[i*2+1],colour,depthtest);
	}
}
static inline uint32 convertColour(const vec4f& colour){
	uint32 result;
	vec4f c = colour*255.0f;
#ifdef ARPHEG_ARCH_BIG_ENDIAN
	result = uint32(c.w)&0xFF | (uint32(c.z)&0xFF)<<8 | (uint32(c.y)&0xFF)<<16 | (uint32(c.x)&0xFF)<<24;
#else
	result = uint32(c.x)&0xFF | (uint32(c.y)&0xFF)<<8 | (uint32(c.z)&0xFF)<<16 | (uint32(c.w)&0xFF)<<24;
#endif
	return result;
}
void Service::line(const mat44f& matrix,vec3f a,vec3f b,const vec4f& colour,bool depthtest){
	//Transform the vertices
	auto m = viewProjection_ * matrix;
	vec4f vertices[2];
	vertices[0] = vec4f(a.x,a.y,a.z,1);
	vertices[1] = vec4f(b.x,b.y,b.z,1);
	for(uint32 i =0;i<2;++i){
		vertices[i] = m*vertices[i]; vertices[i] = vertices[i] * (1.0f/vertices[i].w);
	}

	auto c = convertColour(colour);
	auto geometry = allocateLines(2,2,depthtest);
	
	float* dest = geometry.vertices;
	dest[0] = vertices[0].x; dest[1] = vertices[0].y; dest[2] = vertices[0].z; ((uint32*)dest)[3] = c;
	dest[4] = vertices[1].x; dest[5] = vertices[1].y; dest[6] = vertices[1].z; ((uint32*)dest)[7] = c;
	geometry.indices[0] = uint16(geometry.indexOffset); geometry.indices[1] = uint16(geometry.indexOffset+1);
}

void Service::axis(const mat44f& matrix,bool depthtest){
	line(matrix,vec3f(0,0,0),vec3f(1,0,0),vec4f(1,0,0,1),depthtest);
	line(matrix,vec3f(0,0,0),vec3f(0,1,0),vec4f(0,1,0,1),depthtest);
	line(matrix,vec3f(0,0,0),vec3f(0,0,1),vec4f(0,0,1,1),depthtest);
}
void Service::wireBox(const mat44f& matrix,vec3f min,vec3f max,const vec4f& colour,bool depthtest){
	//Transform the vertices
	auto m = viewProjection_ * matrix;
	vec4f vertices[8];
	vertices[0] = vec4f(min.x,min.y,min.z,1);
	vertices[1] = vec4f(max.x,min.y,min.z,1);
	vertices[2] = vec4f(max.x,max.y,min.z,1);
	vertices[3] = vec4f(min.x,max.y,min.z,1);
	vertices[4] = vec4f(min.x,min.y,max.z,1);
	vertices[5] = vec4f(max.x,min.y,max.z,1);
	vertices[6] = vec4f(max.x,max.y,max.z,1);
	vertices[7] = vec4f(min.x,max.y,max.z,1);
	auto c = convertColour(colour);

	auto geometry = allocateLines(8,24,depthtest);
	uint32 indexOffset = geometry.indexOffset;
	float* dest = geometry.vertices;
	for(uint32 i =0;i<8;++i){
		vertices[i] = m*vertices[i]; vertices[i] = vertices[i] * (1.0f/vertices[i].w);
		dest[0] = vertices[i].x; dest[1] = vertices[i].y; dest[2] = vertices[i].z; ((uint32*)dest)[3] = c;
		dest += kVertexSize/sizeof(float);
	}
	uint16* idx = geometry.indices;
	idx[0] = uint16(indexOffset); idx[1] = uint16(indexOffset+1);
	idx[2] = uint16(indexOffset+1); idx[3] = uint16(indexOffset+2);
	idx[4] = uint16(indexOffset+2); idx[5] = uint16(indexOffset+3);
	idx[6] = uint16(indexOffset+3); idx[7] = uint16(indexOffset);
	idx+=8;
	idx[0] = uint16(indexOffset+4); idx[1] = uint16(indexOffset+5);
	idx[2] = uint16(indexOffset+5); idx[3] = uint16(indexOffset+6);
	idx[4] = uint16(indexOffset+6); idx[5] = uint16(indexOffset+7);
	idx[6] = uint16(indexOffset+7); idx[7] = uint16(indexOffset+4);
	idx+=8;
	idx[0] = uint16(indexOffset); idx[1] = uint16(indexOffset+4);
	idx[2] = uint16(indexOffset+1); idx[3] = uint16(indexOffset+5);
	idx[4] = uint16(indexOffset+2); idx[5] = uint16(indexOffset+6);
	idx[6] = uint16(indexOffset+3); idx[7] = uint16(indexOffset+7);
}
void Service::box(const mat44f& matrix,vec3f min,vec3f max,const vec4f& colour) {
	//TODO
	if(colour.w < 1.0f) trianglesHaveOpacity_ = true;
}

void Service::line(vec2f a,vec2f b,const vec4f& colour){
	line(mat44f::identity(),vec3f(a.x,a.y,0),vec3f(b.x,b.y,0),colour,false);
}
void Service::wireRectangle(vec2f min,vec2f max,const vec4f& colour){
	wireBox(mat44f::identity(),vec3f(min.x,min.y,0),vec3f(max.x,max.y,0),colour,false);
}
void Service::rectangle (vec2f min,vec2f max,const vec4f& colour) {
	box(mat44f::identity(),vec3f(min.x,min.y,0),vec3f(max.x,max.y,0),colour);
}

void Service::render(Pipeline pipeline) {
	//Gather all the thread buffers
#ifndef ARPEG_PLATFORM_NOTHREAD
	using namespace core::bufferArray;
	for(ThreadBufferRegistration* i = begin<ThreadBufferRegistration>(threadBuffers_),*e = end<ThreadBufferRegistration>(threadBuffers_);i<e;++i){
		if(i->isLines) mergeLines(i->vertices,i->indices);
		else mergeTriangles(i->vertices,i->indices);
	}
	threadBuffers_.reset();
#endif

	if(lines_.size() == 0 && triangles_.size() == 0) return;

	auto renderer = services::rendering();
	//Turn off blending
	renderer->bind(blending::disabled());
	//Turn off culling
	rasterization::State rasterState;
	rasterState.cullMode = rasterization::CullNone;
	renderer->bind(rasterState);

	if(vbo_.isNull()){
		core::TypeDescriptor vLayout[2] = { { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TUint8,4 } };
		const char* vSlots[2] = { "position","colour" };
		VertexDescriptor descriptor;
		descriptor.count = 2;
		descriptor.fields = vLayout;
		descriptor.slots = vSlots;

		ibo_  = renderer->create(rendering::Buffer::Index,true,lineIndices_.size(),lineIndices_.bufferBase());
		vbo_  = renderer->create(rendering::Buffer::Vertex,true,lines_.size(),lines_.bufferBase());
		mesh_ = renderer->create(vbo_,ibo_,descriptor);
	} else {
		//Discard + map(slow on ATI? - 'http://hacksoflife.blogspot.ie/2012/04/beyond-glmapbuffer.html')
#ifndef ARPHEG_RENDERING_GLES
		renderer->recreate(rendering::Buffer::Index,ibo_,true,lineIndices_.size()+triangleIndices_.size(),nullptr);
		auto mapping = renderer->map(rendering::Buffer::Index,ibo_);
		if(lineIndices_.size()) memcpy(mapping.data,lineIndices_.bufferBase(),lineIndices_.size());
		if(triangleIndices_.size()) memcpy(((uint8*)mapping.data) + lineIndices_.size(),triangleIndices_.bufferBase(),triangleIndices_.size());
		renderer->unmap(mapping);

		renderer->recreate(rendering::Buffer::Vertex,vbo_,true,lines_.size()+triangles_.size(),nullptr);
		mapping = renderer->map(rendering::Buffer::Vertex,vbo_);
		if(lines_.size()) memcpy(mapping.data,lines_.bufferBase(),lines_.size());
		if(triangles_.size()) memcpy(((uint8*)mapping.data) + lines_.size(),triangles_.bufferBase(),triangles_.size());
		renderer->unmap(mapping);
#else
		renderer->recreate(rendering::Buffer::Index,ibo_,true,lineIndices_.size()+triangleIndices_.size(),nullptr);
		if(lineIndices_.size()) renderer->update(rendering::Buffer::Index,ibo_,0,lineIndices_.bufferBase(),linesIndices_.size());
		if(triangleIndices_.size()) renderer->update(rendering::Buffer::Index,ibo_,lineIndices.size(),triangleIndices_.bufferBase(),triangleIndices_.size());

		renderer->recreate(rendering::Buffer::Vertex,vbo_,true,lines_.size()+triangles_.size(),nullptr);
		if(lines_.size()) renderer->update(rendering::Buffer::Vertex,vbo_,0,lines_.bufferBase(),lines_.size());
		if(triangles_.size()) renderer->update(rendering::Buffer::Vertex,vbo_,lines_.size(),triangles_.bufferBase(),triangles_.size());
#endif
	}
	renderer->bind(pipeline);
	renderer->bind(mesh_,rendering::topology::Line,sizeof(uint16));
	renderer->drawIndexed(0,lineIndices_.size()/sizeof(uint16));

	//Triangles
	if(!triangles_.size()) return;
	rasterState.cullMode = rasterization::CullBack;
	renderer->bind(rasterState);
	if(trianglesHaveOpacity_) renderer->bind(blending::alpha());

}

void Service::servicePreStep(){
	//Reallocate allocators at frame allocator
	auto frameAllocator = services::frameAllocator();
#ifndef ARPEG_PLATFORM_NOTHREAD
	new(&threadBuffers_) core::BufferAllocator( (services::tasking()->workerCount()+2)*sizeof(ThreadBufferRegistration),frameAllocator,core::BufferAllocator::GrowOnOverflow);
#endif
	new(&lines_)       core::BufferAllocator(linesMaxSize,frameAllocator,core::BufferAllocator::GrowOnOverflow);
	new(&lineIndices_) core::BufferAllocator(lineIndicesMaxSize,frameAllocator,core::BufferAllocator::GrowOnOverflow);
	new(&triangles_)   core::BufferAllocator(trianglesMaxSize,frameAllocator,core::BufferAllocator::GrowOnOverflow);
	new(&triangleIndices_) core::BufferAllocator(triangleIndicesMaxSize,frameAllocator,core::BufferAllocator::GrowOnOverflow);
	viewProjection_ = mat44f::identity();
	trianglesHaveOpacity_ = false;
}
void Service::servicePostStep() {
	linesMaxSize = std::max(linesMaxSize,lines_.size());
	lineIndicesMaxSize = std::max(lineIndicesMaxSize,lineIndices_.size());
	trianglesMaxSize = std::max(trianglesMaxSize,triangles_.size());
	triangleIndicesMaxSize = std::max(triangleIndicesMaxSize,triangleIndices_.size());
}
Service::Service(core::Allocator* allocator) :
	lines_    (128,services::frameAllocator()),
	lineIndices_(128,services::frameAllocator()),
	triangles_(128,services::frameAllocator()),
	triangleIndices_(128,services::frameAllocator()),
	threadBuffers_(128,services::frameAllocator())
{
	triangleIndicesMaxSize = linesMaxSize = lineIndicesMaxSize = trianglesMaxSize = 8192;
	ibo_ = vbo_ = Buffer::nullBuffer();
	assertRelease(sizeof(float) == sizeof(uint32));
	trianglesHaveOpacity_ = false;
#ifndef ARPEG_PLATFORM_NOTHREAD
	lineMutex_ = ALLOCATOR_NEW(allocator,core::Mutex);
	triangleMutex_ = ALLOCATOR_NEW(allocator,core::Mutex);
	threadBufferMutex_ = ALLOCATOR_NEW(allocator,core::Mutex);
#endif
}
Service::~Service(){
	if(!vbo_.isNull()){
		auto renderer = services::rendering();
		renderer->release(mesh_);
		renderer->release(vbo_);
		if(!ibo_.isNull()) renderer->release(ibo_);
	}
#ifndef ARPEG_PLATFORM_NOTHREAD
	lineMutex_->~Mutex();
	triangleMutex_->~Mutex();
	threadBufferMutex_->~Mutex();
#endif
}

} }