#include <string.h>
#include "../core/bytes.h"
#include "../core/bufferArray.h"
#include "../core/assert.h"
#include "../core/utf.h"
#include "../services.h"
#include "text.h"
#include "2d.h"
#include "ui.h"

namespace rendering {
namespace text {

static core::TypeDescriptor textVertexLayoutDesc[4] = 
	{ { core::TypeDescriptor::TInt16,2 },{ core::TypeDescriptor::TNormalizedUint16,2 },{ core::TypeDescriptor::TNormalizedUint8,4 },{ core::TypeDescriptor::TNormalizedUint8,4 } };
static core::TypeDescriptor textOutlinesVertexLayoutDesc[5] = 
	{ { core::TypeDescriptor::TInt16,2 },{ core::TypeDescriptor::TNormalizedUint16,2 },{ core::TypeDescriptor::TNormalizedUint8,4 },{ core::TypeDescriptor::TNormalizedUint8,4 },{ core::TypeDescriptor::TNormalizedUint8,4 } };

static const size_t textVertexLayoutDescSize = 2*sizeof(int16) + 2*sizeof(data::Font::Glyph::TextureCoordinate) + sizeof(uint8)*4*2;
static const size_t textOutlinesVertexLayoutDescSize = textVertexLayoutDescSize + sizeof(uint8)*4;

//Vertex layout for rendering text
VertexDescriptor vertexLayout(const data::Font* font){
	auto ftype = font->renderingType() & (~fontType::WithDistanceRendering);
	VertexDescriptor result;
	if(ftype == fontType::Default){
		result.count = 4;result.fields=textVertexLayoutDesc;
	} else if(ftype == fontType::Outlined){
		result.count = 5;result.fields=textOutlinesVertexLayoutDesc;
	} else {
		assert(false && "Unrenderable font type!");
	}
	return result;
}

core::Bytes extractGlyphs(core::Bytes string,data::Font* font,core::BufferAllocator& destination) {
	for(;!string.empty();){
		uint32 codepoint = *string.begin;
		if(codepoint >= 128) {
			auto len = core::utf8::sequenceLength(codepoint);
			//assert(string.length() >= len);
			codepoint = core::utf8::decode(len,string.begin);
			string.begin+=len;
		} else string.begin++;
		if(codepoint == '\n') break;
		auto glyph = font->getGlyph(codepoint);
		if(!glyph){
			glyph = font->getGlyph('?');
			if(!glyph) continue;
		}
		auto element = core::bufferArray::allocate<Glyph>(destination);
		element->glyph = glyph;
	}
	return string;
}

// Direct Font glyph rendering
static void drawGlyph(uint8* vs,uint16* ids,uint16 baseVertex,vec2i min,vec2i max,const data::Font::Glyph::TextureCoordinate* tcoords,uint32 channelMask,uint32 colour,uint32 outlineColour){
	//20*4 vertex bytes per glyph
	draw2D::VertexBuilder builder(vs);
	builder.put(int16(min.x),int16(min.y)).put(tcoords[0],tcoords[1]).put(channelMask).put(colour).put(outlineColour);
	builder.put(int16(max.x),int16(min.y)).put(tcoords[2],tcoords[1]).put(channelMask).put(colour).put(outlineColour);
	builder.put(int16(max.x),int16(max.y)).put(tcoords[2],tcoords[3]).put(channelMask).put(colour).put(outlineColour);
	builder.put(int16(min.x),int16(max.y)).put(tcoords[0],tcoords[3]).put(channelMask).put(colour).put(outlineColour);
	//2*6 = 12 index bytes per glyph
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
}

static void drawGlyph(uint8* vs,uint16* ids,uint16 baseVertex,vec2i min,vec2i max,const data::Font::Glyph::TextureCoordinate* tcoords,uint32 channelMask,uint32 colour){
	//16*4 vertex bytes per glyph
	draw2D::VertexBuilder builder(vs);
	builder.put(int16(min.x),int16(min.y)).put(tcoords[0],tcoords[1]).put(channelMask).put(colour);
	builder.put(int16(max.x),int16(min.y)).put(tcoords[2],tcoords[1]).put(channelMask).put(colour);
	builder.put(int16(max.x),int16(max.y)).put(tcoords[2],tcoords[3]).put(channelMask).put(colour);
	builder.put(int16(min.x),int16(max.y)).put(tcoords[0],tcoords[3]).put(channelMask).put(colour);
	//2*6 = 12 index bytes per glyph
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
}

void drawGlyphs(batching::Geometry& geometry,vec2i position,const data::Font* font,uint32 colour,uint32 outlineColour,const Glyph* glyphs,uint32 count){
	auto ftype = font->renderingType() & (~fontType::WithDistanceRendering);

	uint32 quadSize = ftype == fontType::Outlined? textOutlinesVertexLayoutDescSize*4: textVertexLayoutDescSize*4;
	auto xspace = int(font->spacing_.x);
	uint8* vs = (uint8*) geometry.vertices;
	for(uint32 i = 0;i < count;++i){
		auto glyph = glyphs[i].glyph;
		auto min = position + vec2i(glyph->xoffset,glyph->yoffset); 
		auto max = min + vec2i(glyph->width,glyph->height);
		if(ftype == fontType::Outlined)
			drawGlyph(vs + quadSize*i,geometry.indices+6*i,geometry.indexOffset+4*i,min,max,glyph->textureCoords,0x000000FF,colour,outlineColour);
		else drawGlyph(vs + quadSize*i,geometry.indices+6*i,geometry.indexOffset+4*i,min,max,glyph->textureCoords,0x000000FF,colour);
		position.x += int(glyph->xadvance)+xspace;
	}
	geometry.indexOffset+=4*count; //4X vertices were emmited
	geometry.vertices=(float*)(vs + quadSize*count);
	geometry.indices+=6*count;     //and 6 indices
}

} }