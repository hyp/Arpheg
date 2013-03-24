#include "../core/assert.h"
#include "2d.h"

namespace rendering {
namespace draw2D {

static core::TypeDescriptor triangleColourVertexLayoutDesc[2] = 
	{ { core::TypeDescriptor::TInt16,2 },{ core::TypeDescriptor::TNormalizedUint8,4 } };
static core::TypeDescriptor triangleTextureVertexLayoutDesc[2] = 
	{ { core::TypeDescriptor::TInt16,2 },{ core::TypeDescriptor::TNormalizedUint16,2 } };
static core::TypeDescriptor triangleTextureColourVertexLayoutDesc[3] = 
	{ { core::TypeDescriptor::TInt16,2 },{ core::TypeDescriptor::TNormalizedUint16,2 },{ core::TypeDescriptor::TNormalizedUint8,4 } };

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

void textured::coloured::quad(batching::Geometry& geometry,vec2f min,vec2f max,const uint16* tcoords,uint32 colour){
	//12*4 = 48 vertex bytes per quad
	draw2D::VertexBuilder builder(geometry.vertices);
	builder.put(int16(min.x),int16(min.y)).put(tcoords[0],tcoords[1]).put(colour);
	builder.put(int16(max.x),int16(min.y)).put(tcoords[2],tcoords[1]).put(colour);
	builder.put(int16(max.x),int16(max.y)).put(tcoords[2],tcoords[3]).put(colour);
	builder.put(int16(min.x),int16(max.y)).put(tcoords[0],tcoords[3]).put(colour);
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices = (float*) builder.dest;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}
void textured::quad(batching::Geometry& geometry,vec2f min,vec2f max,const uint16* tcoords){
	//8*4=36 vertex bytes per quad
	draw2D::VertexBuilder builder(geometry.vertices);
	builder.put(int16(min.x),int16(min.y)).put(tcoords[0],tcoords[1]);
	builder.put(int16(max.x),int16(min.y)).put(tcoords[2],tcoords[1]);
	builder.put(int16(max.x),int16(max.y)).put(tcoords[2],tcoords[3]);
	builder.put(int16(min.x),int16(max.y)).put(tcoords[0],tcoords[3]);
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices = (float*) builder.dest;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}
void coloured::quad(batching::Geometry& geometry,vec2f vertices[4],uint32 colours[4]) {
	//8*4=36 vertex bytes per quad
	draw2D::VertexBuilder builder(geometry.vertices);
	for(uint32 i = 0;i<4;++i){
		builder.put(int16(vertices[i].x),int16(vertices[i].y)).put(colours[i]);
	}
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices = (float*) builder.dest;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}
void coloured::quad(batching::Geometry& geometry,vec2f min,vec2f max,uint32 colours[4]) {
	//8*4=36 vertex bytes per quad
	draw2D::VertexBuilder builder(geometry.vertices);
	builder.put(int16(min.x),int16(min.y)).put(colours[0]);
	builder.put(int16(max.x),int16(min.y)).put(colours[1]);
	builder.put(int16(max.x),int16(max.y)).put(colours[2]);
	builder.put(int16(min.x),int16(max.y)).put(colours[3]);
	//2*6 = 12 index bytes per quad
	auto ids = geometry.indices;
	auto baseVertex = uint16(geometry.indexOffset);
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
	geometry.vertices = (float*) builder.dest;
	geometry.indices += 6;
	geometry.indexOffset += 4;
}	

} }
