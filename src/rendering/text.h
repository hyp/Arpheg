#pragma once

#include "types.h"
#include "../data/font/font.h"
#include "geometryBatcher.h"

namespace rendering {
namespace text {

struct Style {
	enum {
		Left,Middle,Right

	};
	data::Font* font;
	uint32 colour;
	uint16 verticalAlignment;
	uint16 horizontalAlignment;

	Style() : horizontalAlignment(Left) { }
};

inline int measureWidth(const Style& style,core::Bytes string){
	int width = 0;
	for(;string.begin < string.end;string.begin++){
		auto glyph = style.font->getGlyph(*string.begin);
		width+=glyph->xadvance;
	}
	return width;
}
inline int measureHeight(const Style& style,core::Bytes string){
	return int(style.font->lineHeight());
}
inline vec2i measure(const Style& style,core::Bytes string){
	return vec2i(measureWidth(style,string),measureHeight(style,string));
}

//
namespace fontType {
	enum {
		Default = 0,
		Outlined = 1,
		Raw = 2,

		WithDistanceRendering = 0x10,
	};
}


//Vertex layout for rendering text
/*
	int16 x2 - position
	uint16 x2(normalized) - texture coordinate
	uint8 x4(normalized) - channel mask
	uint8 x4(normalized) - colour
*/
//Vertex layout for rendering text using outlined fonts
/*
	int16 x2 - position
	uint16 x2(normalized) - texture coordinate
	uint8 x4(normalized) - channel mask
	uint8 x4(normalized) - colour
	uint8 x4(normalized) - border colour
*/
VertexDescriptor vertexLayout(const data::Font* font);

// This structure represents a renderable glyph
struct Glyph {
	const data::Font::Glyph* glyph;
};

//Extracts glyphs from a line of text
//Input: 
//  string - a UTF8 string
//  font - the Font used for rendering
//  destination - the array which stores the extracted glyphs
//Returns:
//  The remaining part of the input string.
core::Bytes extractGlyphs(core::Bytes string,data::Font* font,core::BufferAllocator& destination);

//NB: The assumed vertex layout is:
//vertexLayout() - for standard fonts
//outlinedVertexLayout() - for outlined fonts
void drawGlyphs(batching::Geometry& geometry,vec2i position,const data::Font* font,uint32 colour,uint32 outlineColour,const Glyph* glyphs,uint32 count);

} }
