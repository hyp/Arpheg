#include "../core/assert.h"
#include "2d.h"

namespace rendering {
namespace draw2D {

static core::TypeDescriptor triangleColourVertexLayoutDesc[2] = 
	{ { core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TUint8,4 } };
static core::TypeDescriptor triangleTextureVertexLayoutDesc[2] = 
	{ { core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TFloat,2 } };
static core::TypeDescriptor triangleTextureColourVertexLayoutDesc[3] = 
	{ { core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TUint8,4 } };

VertexDescriptor vertexLayout(uint32 m) {
	VertexDescriptor result;
	using namespace mode;
	if(m & Textured){
		if(m & Coloured){
			result.count =3;result.fields = triangleTextureColourVertexLayoutDesc;
		} else {
			result.count =2;result.fields = triangleTextureVertexLayoutDesc;
		}
		return result;
	} else if(m & Coloured){
		result.count =2;result.fields = triangleColourVertexLayoutDesc;
		return result;
	}

	assert(false && "Unrenderable 2D mode!");
	result.count = 0;
	return result;
}

void textured::coloured::quad(batching::Geometry& geometry,vec2f min,vec2f max,vec2f uvMin,vec2f uvMax,uint32 colour){
	auto vs = geometry.vertices;
	auto uvs = (uint32*)vs;
	//20*4 = 80 vertex bytes per quad
	vs[0] = min.x;vs[1] = min.y;vs[2] = uvMin.x;vs[3] = uvMin.y; uvs[4] = colour;
	vs[5] = max.x;vs[6] = min.y;vs[7] = uvMax.x;vs[8] = uvMin.y; uvs[9] = colour;
	vs[10] = max.x;vs[11] = max.y;vs[12] = uvMax.x;vs[13] = uvMax.y; uvs[14] = colour;
	vs[15] = min.x;vs[16] = max.y;vs[17] = uvMin.x;vs[18] = uvMax.y; uvs[19] = colour;
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices += 20;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}
void textured::quad(batching::Geometry& geometry,vec2f min,vec2f max,vec2f uvMin,vec2f uvMax){
	auto vs = geometry.vertices;
	//16*4 = 64 vertex bytes per quad
	vs[0] = min.x;vs[1] = min.y;vs[2] = uvMin.x;vs[3] = uvMin.y;
	vs[4] = max.x;vs[5] = min.y;vs[6] = uvMax.x;vs[7] = uvMin.y;
	vs[8] = max.x;vs[9] = max.y;vs[10] = uvMax.x;vs[11] = uvMax.y;
	vs[12] = min.x;vs[13] = max.y;vs[14] = uvMin.x;vs[15] = uvMax.y;
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices += 16;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}
void coloured::quad(batching::Geometry& geometry,vec2f vertices[4],uint32 colours[4]) {
	auto vs = geometry.vertices;
	//12*4 = 48 vertex bytes per quad
	for(uint32 i = 0;i<4;++i,vs+=3){
		auto uvs = (uint32*)vs;
		vs[0] = vertices[i].x;vs[1] = vertices[i].y;uvs[2] = colours[i];
	}
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices += 12;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}
void coloured::quad(batching::Geometry& geometry,vec2f min,vec2f max,uint32 colours[4]) {
	auto vs = geometry.vertices;
	auto uvs = (uint32*)vs;
	//12*4 = 48 vertex bytes per quad
	vs[0] = min.x;vs[1] = min.y;uvs[2] = colours[0];
	vs[3] = max.x;vs[4] = min.y;uvs[5] = colours[1];
	vs[6] = max.x;vs[7] = max.y;uvs[8] = colours[2];
	vs[9] = min.x;vs[10] = max.y;uvs[11] = colours[4];
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices += 12;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}	

} }
