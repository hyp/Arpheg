#include <string.h>
#include "../core/bytes.h"
#include "../services.h"
#include "text.h"

namespace rendering {
namespace text {

void draw(GeometryBatcher& batcher,const Style& style,const char* str,vec2f position) {
	//services::rendering()->bind(style.font->pages[0],0);
	if(style.horizontalAlignment != rendering::text::Style::Left){
		core::Bytes string((void*)str,(void*)(str+strlen(str)));
		auto width = measureWidth(style,string);
		if(style.horizontalAlignment == Style::Right){
			position.x-=float(width);
		}
	}
	
	vec2f initial = position;
	//position.y += float(style.font->lineHeight - style.font->base - 3);
	for(;*str != '\0';str++){
		if(*str == '\n'){
			position.y += style.font->lineHeight();
			position.x = initial.x;
		}
		auto glyph = style.font->getGlyph(*str);
		vec2f min = position + vec2f(float(glyph->xoffset),float(glyph->yoffset)); 
		vec2f max = min + vec2f(float(glyph->width),float(glyph->height));
		batcher.quad(min,max,glyph->uvStart,glyph->uvEnd,style.colour);
		position.x += float(glyph->xadvance+style.font->spacing.x);
	}
}

} }