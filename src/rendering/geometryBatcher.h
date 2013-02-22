/**
 Batches geometry with the same vertex layout and texture into one DIP.
*/
#pragma once

#include "rendering.h"

namespace rendering {

	struct GeometryBatcher {
		enum Mode {
			Quads,
			Lines,
		};

		GeometryBatcher();
		void begin (VertexDescriptor vertexLayout,Mode mode = Quads);
		void end   ();
		//Low level output.
		void quad(vec2f min,vec2f max,vec2f uvStart,vec2f uvEnd);
		void quad(vec2f min,vec2f max,vec2f uvStart,vec2f uvEnd,uint32 colour);
		void line(vec2f a,vec2f b,uint32 colour = 0xFF000000);
	private:
		void initialize();
		uint32 vertexSize;
		uint32 vertexBufferSize;
		uint8* buffer,*bufferEnd,*bufferReset;
		Buffer vertices;
		Buffer indices;
		Mode mode;
	};

}