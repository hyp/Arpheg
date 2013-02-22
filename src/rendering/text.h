#pragma once

#include "../resources/font.h"
#include "geometryBatcher.h"

namespace rendering {
namespace text {

struct Style {
	enum {
		Left,Middle,Right

	};
	resources::Font* font;
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

void draw(GeometryBatcher& batcher,const Style& style,const char* str,vec2f position);

} }
