#pragma once

#include "../core/math.h"
#include "../core/typeDescriptor.h"

namespace rendering {

	class Service;
	namespace debug {
		class Service;
	}
	namespace ui    {
		class Service;
	}
	namespace animation {
		class Service;
	}
	namespace batching {
		struct Geometry {
			uint32  indexOffset;
			float*  vertices;
			uint16* indices;
		};
	}


	struct Viewport {
		vec2i position, size;
	};
	

	struct Camera {
		mat44f projection;
		mat44f view;
		mat44f projectionView;

		inline Camera() { }
		Camera(const mat44f& projectionMatrix,const mat44f& viewMatrix);
		inline mat44f calculateMvp(const mat44f& model) const;
	};
	mat44f Camera::calculateMvp(const mat44f& model) const {
		return projectionView * model;
	}

	//A light stored in the hardware ready format.
	struct Light {
		enum { kConstantsVec4fCount = 4 };
		enum { kConstantsSize = kConstantsVec4fCount*sizeof(vec4f) };

		vec4f parameterStorage_[kConstantsVec4fCount];
		
		void makeDirectional(vec3f direction,vec3f diffuse,vec3f specular,vec3f ambient);
		void makePoint      (vec3f position,vec3f diffuse,vec3f specular,vec3f ambient,float radius,float constantAttenuation,float linearAttenuation,float quadraticAttenuation);
		//TODO void makeSpotLight  ();
		
		inline bool isPoint() const;
		inline bool isSpotLight() const;
		inline float radius() const;
		inline vec4f sphere() const;
	};
	inline bool  Light::isPoint() const { return parameterStorage_[0].w > 0.f; }
	inline bool  Light::isSpotLight() const { return parameterStorage_[1].w > 0.f; }
	inline float Light::radius() const { return parameterStorage_[0].w; }
	inline vec4f Light::sphere() const { return parameterStorage_[0]; }

	namespace topology {
		enum Primitive {
			Point,
			Line,
			LineLoop,
			LineStrip,
			Triangle,
			TriangleStrip,
			TriangleFan,
			Patch,
			kMax,
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

		State disabled();
		State alpha();
		State premultipliedAlpha();
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
		core::TypeDescriptor* fields;
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
		static VertexDescriptor positionAs3Float_normalAs3Float_texcoordAs2Float();
	};
	namespace texture {
		enum Format {
			R_8,
			RG_88,
			RGB_888,
			RGBA_8888,
			UINT_R_16,
			UINT_RG_1616,
			UINT_RGB_161616,
			UINT_RGBA_16161616,
			UINT_R_32,
			UINT_RG_3232,
			UINT_RGB_323232,
			UINT_RGBA_32323232,
			FLOAT_R_32,
			FLOAT_RG_3232,
			FLOAT_RGB_323232,
			FLOAT_RGBA_32323232,
			FLOAT_R_16,
			FLOAT_RG_1616,
			FLOAT_RGB_161616,
			FLOAT_RGBA_16161616,
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

#ifdef ARPHEG_RENDERING_GL
	#include "opengl/types.h"
#endif
