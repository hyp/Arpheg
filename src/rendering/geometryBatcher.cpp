#include <limits>
#include "../core/assert.h"
#include "../services.h"
#include "geometryBatcher.h"

namespace rendering{ 
GeometryBatcher::GeometryBatcher(){
	vertexSize = 0;
	vertexBufferSize = 0;
	buffer = bufferEnd = bufferReset = nullptr;
}
void GeometryBatcher::initialize() {
	auto renderer = services::rendering();
	uint32 maxQuads = 2048;
	vertexBufferSize = maxQuads * 4 * vertexSize;
	size_t indexBufferSize = maxQuads*6*sizeof(uint16);
	assert(maxQuads*4 <= std::numeric_limits<uint16>::max());

	//TODO scratch allocator
	//Optimize for quads
	vertices = renderer->create(rendering::Buffer::Vertex,true,vertexBufferSize);
	auto indexData = (uint16*)core::memory::globalAllocator()->allocate(indexBufferSize);
	auto indexD = indexData;
	for(uint32 i = 0; i < maxQuads;i++,indexData+=6){
		indexData[0] = i*4;
		indexData[1] = i*4+1;
		indexData[2] = i*4+2;
		indexData[3] = i*4+1;
		indexData[4] = i*4+2;
		indexData[5] = i*4+3;
	}
	indices = renderer->create(rendering::Buffer::Index,false,indexBufferSize,indexD);
	core::memory::globalAllocator()->deallocate(indexD);
}
void GeometryBatcher::begin(VertexDescriptor vertexLayout,Mode mode) {
	vertexSize = 0;
	for(auto i = 0; i< vertexLayout.count;i++)
		vertexSize += vertexLayout.fields[i].size();
	this->mode = mode;

	assert(vertexSize != 0);
	if(vertexBufferSize == 0) initialize();

	if(!buffer){
		buffer = (uint8*)services::frameAllocator()->allocate(vertexBufferSize,16);
		bufferEnd = buffer + vertexBufferSize;
		bufferReset = buffer;
	} else buffer = bufferReset;
}
void GeometryBatcher::end(){
	auto count = uint32(buffer-bufferReset)/vertexSize;
	if(!count) return;
	auto renderer = services::rendering();

	renderer->update(Buffer::Vertex,vertices,0,bufferReset,size_t(buffer - bufferReset));
	renderer->bind(rendering::topology::Triangle);
	renderer->bindVertices(vertices);
	renderer->bindIndices(indices,sizeof(uint16));
	renderer->drawIndexed(0,count/4*6);
	buffer = bufferReset;
}
void GeometryBatcher::quad(vec2f min,vec2f max,vec2f uvStart,vec2f uvEnd){
	assert(mode == Quads);
	if(buffer + vertexSize*4 >= bufferEnd)
		end();

	auto vertices = (vec2f*)buffer;
	vertices[0] = vec2f(min.x,min.y);
	vertices[1] = uvStart;
	vertices = (vec2f*)(buffer+vertexSize);
	vertices[0] = vec2f(max.x,min.y);
	vertices[1] = vec2f(uvEnd.x,uvStart.y);
	vertices = (vec2f*)(buffer+vertexSize*2);
	vertices[0] = vec2f(min.x,max.y);
	vertices[1] = vec2f(uvStart.x,uvEnd.y);
	vertices = (vec2f*)(buffer+vertexSize*3);
	vertices[0] = max;
	vertices[1] = uvEnd;

	buffer = buffer + vertexSize*4;
}
void GeometryBatcher::quad(vec2f min,vec2f max,vec2f uvStart,vec2f uvEnd,uint32 colour) {
	assert(mode == Quads);
	if(buffer + vertexSize*4 >= bufferEnd)
		end();

	auto vertices = (vec2f*)buffer;
	vertices[0] = vec2f(min.x,min.y);
	vertices[1] = uvStart;
	*(uint32*)(vertices+2) = colour;
	vertices = (vec2f*)(buffer+vertexSize);
	vertices[0] = vec2f(max.x,min.y);
	vertices[1] = vec2f(uvEnd.x,uvStart.y);
	*(uint32*)(vertices+2) = colour;
	vertices = (vec2f*)(buffer+vertexSize*2);
	vertices[0] = vec2f(min.x,max.y);
	vertices[1] = vec2f(uvStart.x,uvEnd.y);
	*(uint32*)(vertices+2) = colour;
	vertices = (vec2f*)(buffer+vertexSize*3);
	vertices[0] = max;
	vertices[1] = uvEnd;
	*(uint32*)(vertices+2) = colour;

	buffer = buffer + vertexSize*4;
}
void GeometryBatcher::line(vec2f a,vec2f b,uint32 colour) {
	assert(mode == Lines);
	if(buffer + vertexSize*2 >= bufferEnd)
		end();

	auto vertices = (vec2f*)buffer;
	vertices[0] = vec2f(a.x,a.y);
	*(uint32*)(vertices+1) = colour;
	vertices = (vec2f*)(buffer+vertexSize);
	vertices[0] = vec2f(b.x,b.y);
	*(uint32*)(vertices+1) = colour;	

	buffer = buffer + vertexSize*2;
}

}