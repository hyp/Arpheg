#include <string.h>
#include "../../../core/assert.h"
#include "../../../core/text.h"
#include "../../../core/io.h"
#include "../../../rendering/text.h"
#include "reader.h"

namespace data {
namespace intermediate {
namespace font {

//Imports Font from the text config based on angelcode bitmap font generator
//TODO kerning.

struct FontParser : core::text::Tokenizer {
	struct Common {
		int lineHeight,base,width,height,pages,packed;
		int redChannel,greenChannel,blueChannel,alphaChannel;
	};
	struct Char {
		int id;
		int x,y,width,height;
		int xoffset,yoffset;
		int xadvance;
		int page;
		int channel;
	};

	Reader* reader;
	Font& font;
	Common common;
	int prevCharId;

	void wildcard(){
		if(matchToken("=")){
			do {
				if(matchToken("\"")){
					char buffer[16];
					readString(buffer,sizeof(buffer),'"');
				} else expectInteger();
			} while(matchToken(","));
		}
	}
	void integer(const char* prop,int& value){
		if(matchAsciiIdentifier(prop)){
			expectToken("=");
			value = expectInteger();
		}
	}
	void integers(const char* prop,int* values,int amount){
		if(matchAsciiIdentifier(prop)){
			expectToken("=");
			for(int i =0;i <amount;i++){
				if(i!=0) expectToken(",");
				values[i] =expectInteger();
			}
		}
	}
	void string(const char* prop,char* buffer,int length){
		if(matchAsciiIdentifier(prop)){
			expectToken("=");
			expectToken("\"");
			readString(buffer,length,'"');
		}
	}
	void parseInfo(){
		int spacing[2] = { 0 };
		int outline = 0;
		while(!empty()){
			skipSpaces();
			getAsciiIdentifier();
			integers("spacing",spacing,2);
			integer("outline",outline);
			wildcard();
		}
		font.spacing_ = vec2f(0.0f,float(spacing[1]+outline));
		font.outline_ = float(outline);
		if(font.outline_ > 0.0){
			font.renderType_ = rendering::text::fontType::Outlined;
		} else {
			font.renderType_ = rendering::text::fontType::Default;
		}
	}
	void parseCommon(){
		while(!empty()){
			skipSpaces();
			getAsciiIdentifier();

			integer("lineHeight",common.lineHeight);
			integer("base",common.base);
			integer("scaleW",common.width);
			integer("scaleH",common.height);
			integer("pages",common.pages);
			integer("packed",common.packed);
			integer("alphaChnl",common.alphaChannel);
			integer("redChnl",common.redChannel);
			integer("greenChnl",common.greenChannel);
			integer("blueChnl",common.blueChannel);
			wildcard();
		}
		font.base_ = float(common.base);
		font.lineHeight_ = float(common.lineHeight);
	}
	void parsePage(){
		int id = -1;
		char name[260];
		while(!empty()){
			skipSpaces();
			getAsciiIdentifier();
					
			integer("id",id);
			string("file",name,sizeof(name));

			wildcard();
		}
		if(id != -1){
			assert(font.pageCount_ < Font::kMaxPages);
			reader->processTextureRequest(core::Bytes(name,strlen(name)));
		}
	}
	void parseChar(){
		Char character = { 0 };
		while(!empty()){
			skipSpaces();
			getAsciiIdentifier();

			integer("id",character.id);
			integer("x",character.x);
			integer("y",character.y);
			integer("width",character.width);
			integer("height",character.height);
			integer("xoffset",character.xoffset);
			integer("yoffset",character.yoffset);
			integer("xadvance",character.xadvance);
			integer("page",character.page);
			integer("chnl",character.channel);
			wildcard();
		}
		Font::Glyph glyph;
		glyph.texture= character.page;
		glyph.xadvance = character.xadvance;
		glyph.xoffset = character.xoffset;
		glyph.yoffset = character.yoffset;
		glyph.width   = character.width;
		glyph.height  = character.height;
		glyph.uvStart = vec2f(float(character.x)/float(common.width), float(character.y)/float(common.height));
		glyph.uvEnd   = vec2f(float(character.x+character.width)/float(common.width), float(character.y + character.height)/float(common.height));
		assert(character.id <= 255);
		font.glyphs_[character.id] = glyph;
	}
	FontParser(Reader* r,Font& fnt) : reader(r),font(fnt),core::text::Tokenizer(core::Bytes(nullptr,(size_t)0),core::text::Tokenizer::Trim) {
		prevCharId = -1;
	}
	void parseLine(core::Bytes line){
		data_ = line;
		getAsciiIdentifier();
		if(matchAsciiIdentifier("char")) parseChar();
		else if(matchAsciiIdentifier("common")) parseCommon();
		else if(matchAsciiIdentifier("page")) parsePage();
		else if(matchAsciiIdentifier("info")) parseInfo();
	}
};

void Reader::load(core::Bytes data){
	FontParser parser(this,result);
	for(core::text::Lines lines(data);!lines.empty();)
		parser.parseLine(lines.getLineTrimmedSkipEmpty());
}

} } }