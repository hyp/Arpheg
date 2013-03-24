#include "../../rendering/text.h"
#include "font.h"


namespace data {

Font::Font(){
	pageCount_ = 0;
	lineHeight_ = 0.0f;
	base_ = 0.0f;
	outline_ = 0.0f;
	renderType_ = rendering::text::fontType::Default;
}
static inline void flipY(Font::Glyph& glyph){
	glyph.textureCoords[1] = Font::Glyph::textureCoordinateOne() - glyph.textureCoords[1];
	glyph.textureCoords[3] = Font::Glyph::textureCoordinateOne() - glyph.textureCoords[3];
}
void Font::flipYTexcoord() {
	for(uint32 i = 0;i<256;++i) flipY(glyphs_[i]);
}

}