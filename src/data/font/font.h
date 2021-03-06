#pragma once

#include "../../core/math.h"
#include "../types.h"

namespace data {

struct Font {
	struct Glyph {
		typedef normalizedUint16::Type TextureCoordinate;
		static inline TextureCoordinate textureCoordinateOne() {
			return normalizedUint16::one;
		}
		static inline TextureCoordinate textureCoordinate(float x){
			return normalizedUint16::make(x);
		}

		int16  xadvance,xoffset,yoffset;
		int16  width,height;
		TextureCoordinate textureCoords[4];
	};

	enum { kMaxPages = 4 };

	Font();
	void flipYTexcoord();
	inline const Glyph* getGlyph(uint32 codepoint);
	inline uint32 renderingType() const;
	inline float lineHeight();
	inline float base();
	inline float outline();
	
	uint32 pageCount_;
	uint32 renderType_;
	rendering::Texture2D pages_[kMaxPages];
	float lineHeight_,base_,outline_;
	vec2f spacing_;
	Glyph glyphs_[256];	
};

inline uint32 Font::renderingType() const { return renderType_; }
inline float Font::lineHeight(){ return lineHeight_; }
inline float Font::base()      { return base_;       }
inline float Font::outline()   { return outline_;    }
inline const Font::Glyph* Font::getGlyph(uint32 codepoint){
	return glyphs_+codepoint;
}

}