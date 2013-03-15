#include <string.h>
#include "../core/bytes.h"
#include "../core/bufferArray.h"
#include "../core/assert.h"
#include "../core/utf.h"
#include "../services.h"
#include "text.h"
#include "ui.h"

namespace rendering {
namespace text {

static core::TypeDescriptor textVertexLayoutDesc[4] = 
	{ { core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TUint8,4 },{ core::TypeDescriptor::TUint8,4 } };
static core::TypeDescriptor textOutlinesVertexLayoutDesc[5] = 
	{ { core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TFloat,2 },{ core::TypeDescriptor::TUint8,4 },{ core::TypeDescriptor::TUint8,4 },{ core::TypeDescriptor::TUint8,4 } };

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
static void drawGlyph(float* vs,uint16* ids,uint16 baseVertex,vec2f min,vec2f max,vec2f uvMin,vec2f uvMax,uint32 channelMask,uint32 colour,uint32 outlineColour){
	auto uvs = (uint32*)vs;
	//28*4 = 112 vertex bytes per glyph
	vs[0] = min.x;vs[1] = min.y;vs[2] = uvMin.x;vs[3] = uvMin.y; uvs[4] = channelMask; uvs[5] = colour; uvs[6] = outlineColour;
	vs[7] = max.x;vs[8] = min.y;vs[9] = uvMax.x;vs[10] = uvMin.y; uvs[11] = channelMask; uvs[12] = colour; uvs[13] = outlineColour;
	vs[14] = max.x;vs[15] = max.y;vs[16] = uvMax.x;vs[17] = uvMax.y; uvs[18] = channelMask; uvs[19] = colour; uvs[20] = outlineColour;
	vs[21] = min.x;vs[22] = max.y;vs[23] = uvMin.x;vs[24] = uvMax.y; uvs[25] = channelMask; uvs[26] = colour; uvs[27] = outlineColour;
	//2*6 = 12 index bytes per glyph
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
}

static void drawGlyph(float* vs,uint16* ids,uint16 baseVertex,vec2f min,vec2f max,vec2f uvMin,vec2f uvMax,uint32 channelMask,uint32 colour){
	auto uvs = (uint32*)vs;
	//24*4 = 96 vertex bytes per glyph
	vs[0] = min.x;vs[1] = min.y;vs[2] = uvMin.x;vs[3] = uvMin.y; uvs[4] = channelMask; uvs[5] = colour;
	vs[6] = max.x;vs[7] = min.y;vs[8] = uvMax.x;vs[9] = uvMin.y; uvs[10] = channelMask; uvs[11] = colour;
	vs[12] = max.x;vs[13] = max.y;vs[14] = uvMax.x;vs[15] = uvMax.y; uvs[16] = channelMask; uvs[17] = colour;
	vs[18] = min.x;vs[19] = max.y;vs[20] = uvMin.x;vs[21] = uvMax.y; uvs[22] = channelMask; uvs[23] = colour;
	//2*6 = 12 index bytes per glyph
	ids[0] = baseVertex;ids[1] = baseVertex+1;ids[2] = baseVertex+2;
	ids[3] = baseVertex+2;ids[4] = baseVertex+3;ids[5] = baseVertex;
}

void drawGlyphs(batching::Geometry& geometry,vec2f position,const data::Font* font,uint32 colour,uint32 outlineColour,const Glyph* glyphs,uint32 count){
	auto ftype = font->renderingType() & (~fontType::WithDistanceRendering);

	uint32 quadFloatCount = ftype == fontType::Outlined? 28: 24;
	auto xspace = font->spacing_.x;
	for(uint32 i = 0;i < count;++i){
		auto glyph = glyphs[i].glyph;
		vec2f min = position + vec2f(float(glyph->xoffset),float(glyph->yoffset)); 
		vec2f max = min + vec2f(float(glyph->width),float(glyph->height));
		if(ftype == fontType::Outlined)
			drawGlyph(geometry.vertices+quadFloatCount*i,geometry.indices+6*i,geometry.indexOffset+4*i,min,max,glyph->uvStart,glyph->uvEnd,0x000000FF,colour,outlineColour);
		else drawGlyph(geometry.vertices+quadFloatCount*i,geometry.indices+6*i,geometry.indexOffset+4*i,min,max,glyph->uvStart,glyph->uvEnd,0x000000FF,colour);
		position.x += float(glyph->xadvance+xspace);
	}
	geometry.indexOffset+=4*count; //4X vertices were emmited
	geometry.vertices+=quadFloatCount*count;   //each quad had 24/28 float vertices
	geometry.indices+=6*count;     //and 6 indices
}

} }