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
void Font::flipYTexcoord() {
	for(uint32 i = 0;i<256;++i){
		glyphs_[i].uvStart.y = 1.0f - glyphs_[i].uvStart.y;
		glyphs_[i].uvEnd.y   = 1.0f - glyphs_[i].uvEnd.y;
	}
}

}