#include <limits>
#include <string.h>
#include "../core/assert.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../application/tasking.h"
#include "../services.h"
#include "rendering.h"
#include "debug.h"
#include "opengl/gl.h"

//TODO triangles

namespace rendering {
namespace debug {

//Position + colour
static const size_t kVertexSize = sizeof(float)*3 + sizeof(uint8)*4;

Service::GeometryDestination Service::allocateLines(uint32 vertexCount,uint32 indexCount,bool depthtest){
	core::BufferAllocator& vertices = depthtest? linesDepthTest_: lines_;
	GeometryDestination result = { vertices.size()/kVertexSize,(float*)vertices.allocate(kVertexSize*vertexCount),nullptr };
	return result;
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
	vec4f vertices[2];
	vertices[0] = vec4f(a.x,a.y,a.z,1);
	vertices[1] = vec4f(b.x,b.y,b.z,1);
	for(uint32 i =0;i<2;++i){
		vertices[i] = matrix*vertices[i];
	}

	auto c = convertColour(colour);
	auto geometry = allocateLines(2,2,depthtest);
	
	float* dest = geometry.vertices;
	dest[0] = vertices[0].x; dest[1] = vertices[0].y; dest[2] = vertices[0].z; ((uint32*)dest)[3] = c;
	dest[4] = vertices[1].x; dest[5] = vertices[1].y; dest[6] = vertices[1].z; ((uint32*)dest)[7] = c;
}
void Service::line(vec3f a,vec3f b,uint32 colour,bool depthtest) {
	auto geometry = allocateLines(2,2,depthtest);
	float* dest = geometry.vertices;
	dest[0] = a.x; dest[1] = a.y; dest[2] = a.z; ((uint32*)dest)[3] = colour;
	dest[4] = b.x; dest[5] = b.y; dest[6] = b.z; ((uint32*)dest)[7] = colour;
}
void Service::node(const mat44f& matrix,bool depthtest){
	line(matrix,vec3f(0,0,0),vec3f(1,0,0),vec4f(1,0,0,1),depthtest);
	line(matrix,vec3f(0,0,0),vec3f(0,1,0),vec4f(0,1,0,1),depthtest);
	line(matrix,vec3f(0,0,0),vec3f(0,0,1),vec4f(0,0,1,1),depthtest);
}
void Service::box(const mat44f& matrix,vec3f min,vec3f max,const vec4f& colour,bool depthtest){
	//Transform the vertices
	vec4f vertices[8];
	vec3f vs[8];
	vertices[0] = matrix*vec4f(min.x,min.y,min.z,1);
	vertices[1] = matrix*vec4f(max.x,min.y,min.z,1);
	vertices[2] = matrix*vec4f(max.x,max.y,min.z,1);
	vertices[3] = matrix*vec4f(min.x,max.y,min.z,1);
	vertices[4] = matrix*vec4f(min.x,min.y,max.z,1);
	vertices[5] = matrix*vec4f(max.x,min.y,max.z,1);
	vertices[6] = matrix*vec4f(max.x,max.y,max.z,1);
	vertices[7] = matrix*vec4f(min.x,max.y,max.z,1);
	for(uint32 i = 0;i<8;++i) vs[i] = vertices[i].xyz();
	auto c = convertColour(colour);

	line(vs[0],vs[1],c,depthtest);line(vs[1],vs[2],c,depthtest);
	line(vs[2],vs[3],c,depthtest);line(vs[3],vs[0],c,depthtest);

	line(vs[4],vs[5],c,depthtest);line(vs[5],vs[6],c,depthtest);
	line(vs[6],vs[7],c,depthtest);line(vs[7],vs[4],c,depthtest);
	
	line(vs[0],vs[4],c,depthtest);line(vs[1],vs[5],c,depthtest);
	line(vs[2],vs[6],c,depthtest);line(vs[3],vs[7],c,depthtest);
}
void Service::sphere(const mat44f& matrix,vec3f position,float radius,const vec4f& colour,bool depthtest) {
	float deltaTheta = math::toRadians(float(45));
	auto c = convertColour(colour);
	for (uint32 i = 0; i < 360; i += 45){

		float theta = deltaTheta * float(i);
		float a = radius * sinf(theta);
		float b = radius * cosf(theta);
		float c = radius * sinf(theta + deltaTheta);
		float d = radius * cosf(theta + deltaTheta);
		vec3f start, end;
        
		start = position + vec3f(a, b, 0.0f);
		end = position + vec3f(c, d, 0.0f);
		line(start, end, c, depthtest);
		start = position + vec3f(a, 0.0f, b);
		end = position + vec3f(c, 0.0f, d);
		line(start, end, c, depthtest);
		start = position + vec3f(0.0f, a, b);
		end = position + vec3f(0.0f, c, d);
		line(start, end, c, depthtest);
	}
}
void Service::skeleton(const mat44f& matrix,const data::Mesh* mesh,const data::Transformation3D* nodes,const vec4f& colour,bool depthtest) {
	auto count = mesh->skeletonNodeCount();
	if(!count) return;
	for(size_t i = 1; i < count;++i){
		auto parent = mesh->skeletonHierarchy()[i];
		line(matrix,nodes[parent].translationComponent(),nodes[i].translationComponent(),colour,depthtest);
	}
}

void Service::line(vec2f a,vec2f b,const vec4f& colour){
	line(mat44f::identity(),vec3f(a.x,a.y,0),vec3f(b.x,b.y,0),colour,false);
}
void Service::rectangle(vec2f min,vec2f max,const vec4f& colour){
	box(mat44f::identity(),vec3f(min.x,min.y,0),vec3f(max.x,max.y,0),colour,false);
}

void Service::pipeline(const data::Pipeline* pipeline){
	this->pipeline(pipeline->pipeline());
}
void Service::pipeline(Pipeline pipeline){
	pipeline_ = pipeline;
	pipelineModelView = Pipeline::Constant("mvp");
}
void Service::render(const mat44f& viewProjection) {
	if(lines_.size() == 0 && linesDepthTest_.size() == 0) return;

	glDisable(GL_DEPTH_TEST);

	auto renderer = services::rendering();
	//Turn off blending
	renderer->bind(blending::disabled());
	//Turn off culling
	rasterization::State rasterState;
	rasterState.cullMode = rasterization::CullNone;
	renderer->bind(rasterState);

	bool justCreated = false;
	if(vbo_.isNull()){
		core::TypeDescriptor vLayout[2] = { { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TUint8,4 } };
		const char* vSlots[2] = { "position","colour" };
		VertexDescriptor descriptor;
		descriptor.count = 2;
		descriptor.fields = vLayout;
		descriptor.slots = vSlots;

		vbo_  = renderer->create(rendering::Buffer::Vertex,true,lines_.size()+linesDepthTest_.size(),nullptr);
		mesh_ = renderer->create(vbo_,ibo_,descriptor);
		justCreated = true;
	}
	
	//Discard + map(slow on ATI? - 'http://hacksoflife.blogspot.ie/2012/04/beyond-glmapbuffer.html')
#ifndef ARPHEG_RENDERING_GLES
	if(!justCreated) renderer->recreate(rendering::Buffer::Vertex,vbo_,true,lines_.size()+linesDepthTest_.size(),nullptr);
	auto mapping = renderer->map(rendering::Buffer::Vertex,vbo_);
	if(lines_.size())          memcpy(mapping.data,lines_.bufferBase(),lines_.size());
	if(linesDepthTest_.size()) memcpy(((uint8*)mapping.data) + lines_.size(),linesDepthTest_.bufferBase(),linesDepthTest_.size());
	renderer->unmap(mapping);
#else
	if(!justCreated) renderer->recreate(rendering::Buffer::Vertex,vbo_,true,lines_.size()+linesDepthTest_.size(),nullptr);
	if(lines_.size()) renderer->update(rendering::Buffer::Vertex,vbo_,0,lines_.bufferBase(),lines_.size());
	if(linesDepthTest_.size()) renderer->update(rendering::Buffer::Vertex,vbo_,lines_.size(),linesDepthTest_.bufferBase(),linesDepthTest_.size());
#endif
	
	renderer->bind(pipeline_);
	renderer->bind(pipelineModelView,viewProjection);
	renderer->bind(mesh_,rendering::topology::Line);
	renderer->draw(0,lines_.size()/kVertexSize);

	glEnable(GL_DEPTH_TEST);

	if(!linesDepthTest_.size()) return;

	renderer->draw(lines_.size()/kVertexSize,linesDepthTest_.size()/kVertexSize);
}

void Service::servicePreStep(){
	//Reallocate allocators at frame allocator
	auto frameAllocator = services::frameAllocator();

	new(&lines_)            core::BufferAllocator(linesMaxSize,frameAllocator,core::BufferAllocator::GrowOnOverflow);
	new(&linesDepthTest_)   core::BufferAllocator(linesDepthTestMaxSize,frameAllocator,core::BufferAllocator::GrowOnOverflow);
	viewProjection_ = mat44f::identity();
}
void Service::servicePostStep() {
	linesMaxSize = std::max(linesMaxSize,lines_.size());
	linesDepthTestMaxSize = std::max(linesDepthTestMaxSize,linesDepthTest_.size());
}
Service::Service(core::Allocator* allocator) :
	lines_    (128,services::frameAllocator()),
	linesDepthTest_(128,services::frameAllocator()),
	pipelineModelView("") {
	linesDepthTestMaxSize = linesMaxSize = 8192;
	ibo_ = vbo_ = Buffer::nullBuffer();
	assertRelease(sizeof(float) == sizeof(uint32));
}
Service::~Service(){
	if(!vbo_.isNull()){
		auto renderer = services::rendering();
		renderer->release(mesh_);
		renderer->release(vbo_);
		if(!ibo_.isNull()) renderer->release(ibo_);
	}
}

} }