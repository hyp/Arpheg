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

	//The textured coloured triangles batch has the following vertex layout:
	//float x2 - position
	//float x2 - texture coordinate
	//uint8 x4(normalized) - colour
	VertexDescriptor vertexLayout(uint32 mode);

	namespace textured {
		namespace coloured {
			void quad(batching::Geometry& geometry,vec2f min,vec2f max,vec2f uvMin,vec2f uvMax,uint32 colour);
		}
		void quad(batching::Geometry& geometry,vec2f min,vec2f max,vec2f uvMin,vec2f uvMax);
	}
	namespace coloured {
		void quad(batching::Geometry& geometry,vec2f vertices[4],uint32 colours[4]);
		void quad(batching::Geometry& geometry,vec2f min,vec2f max,uint32 colours[4]);
	}
} }