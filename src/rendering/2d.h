#pragma once

#include "types.h"

namespace rendering {
namespace draw2D {
	namespace mode {
		enum {
			Textured = 0x1,
			Coloured = 0x2,
		};
	}

	struct VertexBuilder {
		uint8* dest;

		inline VertexBuilder(void* ptr) : dest((uint8*)ptr) {}
		inline VertexBuilder& put(float x,float y){
			auto fd = (float*) dest;
			fd[0]=x;fd[1] = y;
			dest+=sizeof(float)*2;
			return *this;
		}
		inline VertexBuilder& put(int16 x,int16 y){
			auto fd = (int16*) dest;
			fd[0]=x;fd[1] = y;
			dest+=sizeof(int16)*2;
			return *this;
		}
		inline VertexBuilder& put(uint16 x,uint16 y){
			auto fd = (uint16*) dest;
			fd[0]=x;fd[1] = y;
			dest+=sizeof(uint16)*2;
			return *this;
		}
		inline VertexBuilder& put(uint32 x){
			((uint32*)dest)[0] = x;
			dest+=sizeof(uint32);
			return *this;
		}
	};

	//The textured coloured triangles batch has the following vertex layout:
	//float  x2 - position
	//uint16 x2(normalized) - texture coordinate
	//uint8  x4(normalized) - colour
	VertexDescriptor vertexLayout(uint32 mode);

	namespace textured {
		namespace coloured {
			void quad(batching::Geometry& geometry,vec2f min,vec2f max,const uint16* tcoords,uint32 colour);
		}
		void quad(batching::Geometry& geometry,vec2f min,vec2f max,const uint16* tcoords);
	}

namespace positionInt16 {

	//The textured coloured triangles batch has the following vertex layout:
	//int16 x4 - position + instance properties(texture, etc)
	//uint16 x2(normalized) - texture coordinate
	//uint8  x4(normalized) - colour
	VertexDescriptor vertexLayout(uint32 mode);

	namespace textured {
		namespace coloured {
			inline void quad(batching::Geometry& geometry,vec2i min,vec2i max,const uint16* tcoords,uint32 colour);
			void quad(batching::Geometry& geometry,vec2i min,vec2i max,const uint16* tcoords,uint32 colours[4]);
		}
	}
	inline void textured::coloured::quad(batching::Geometry& geometry,vec2i min,vec2i max,const uint16* tcoords,uint32 colour){
		uint32 colours[4] = {colour,colour,colour,colour};
		quad(geometry,min,max,tcoords,colours);
	}

} } }