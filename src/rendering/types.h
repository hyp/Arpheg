#pragma once

#include "../core/math.h"
#include "../core/typeDescriptor.h"

namespace rendering {

	struct Viewport {
		vec2i position, size;
	};
	namespace topology {
		enum Primitive {
			Point,
			Line,
			LineLoop,
			LineStrip,
			Triangle,
			TriangleStrip,
			TriangleFan,
		};
	}
	namespace blending {
		enum Blend {
			Zero,One,
			SrcColor,InvertedSrcColor,
			SrcAlpha,InvertedSrcAlpha,
			DestAlpha,InvertedDestAlpha,
			DestColor,InvertedDestColor,
		};
		enum Op    {
			Add,Subtract,ReverseSubtract,Min,Max
		};
		struct State {
			bool enabled;
			Blend srcRgb, srcAlpha;
			Blend destRgb, destAlpha;
			Op colorOp, alphaOp;

			inline State() : enabled(false) { }
			inline State(Blend src,Blend dest,Op op = Add) {
				enabled = true;
				srcRgb  = srcAlpha = src;
				destRgb = destAlpha = dest;
				colorOp = alphaOp = op;
			}
		};
	}
	namespace rasterization {
		enum FillMode {
			Solid,Wireframe
		};
		enum CullMode {
			CullNone,CullFront,CullBack
		};

		struct State {
			uint8 fillMode;
			uint8 cullMode;
			uint8 isFrontFaceCounterClockwise;

			inline State() : fillMode(Solid),cullMode(CullNone),isFrontFaceCounterClockwise(0) { }
		};
	}
	struct VertexDescriptor {
		CoreTypeDescriptor* fields;
		uint32 count;
		const char** slots;
		
		static VertexDescriptor positionAs2Float();
		static VertexDescriptor positionAs3Float();
		static VertexDescriptor positionAs4Float();
		static VertexDescriptor positionAs2Float_texcoordAs2Float();
		static VertexDescriptor positionAs3Float_texcoordAs2Float();
		static VertexDescriptor positionAs2Float_texcoordAs2Float_colourAs4Bytes();
		static VertexDescriptor positionAs3Float_normalAs3Float();
		static VertexDescriptor positionAs4Float_normalAs4Float();
	};
	namespace texture {
		enum Format {
			R_8,
			RG_88,
			RGB_888,
			RGBA_8888,
			UINT_RG_1616,
			UINT_RGB_161616,
			UINT_RGBA_16161616,
			DXT1,
			DXT5,

			RENDER_TARGET = 0x8000,
		};
		struct Descriptor1D {
			int32 size;
			uint32 format;
		};
		struct Descriptor2D {
			int32  width; //size on the x-axis
			int32  height;//size on the y-axis
			uint32 format;
		};
		struct Descriptor3D {
			int32 width; //size on the x-axis
			int32 height;//size on the y-axis
			int32 length;//size on the z-axis
			uint32 format;
		};

		struct SamplerDescriptor {
			enum Filter {
				MinPoint    = 0,
				MinLinear   = 1,
				MagPoint    = 0,
				MagLinear   = 2,
				MipPoint    = 4,
				MipLinear   = 8,
			};
			enum AddressMode {
				Clamp,Repeat,Mirror
			};
			//Layout - 8 bits filter,3x4 bits u,v,w adressing,8 bits - anisotropy,4 bits - comparison(TODO)
			uint32 filter;
			uint8  adressModes[3];
			uint32 maxAnisotropy;
			float  mipLodBias;

			inline SamplerDescriptor() : filter(MinPoint|MagPoint),maxAnisotropy(0),mipLodBias(0.0f) {}
		};
	}

	struct Limits {
		vec2i maxTexture2DSize;
		int32 maxTexture3DSize;
		int32 maxTextureArrayLayers;

		size_t maxConstantBufferSize;
	};
}

#ifdef PLATFORM_RENDERING_GL
	#include "opengl/types.h"
#endif
