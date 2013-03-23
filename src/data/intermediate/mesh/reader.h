#pragma once

#include "../../core/memoryTypes.h"
#include "../../core/math.h"
#include "../../core/bytes.h"
#include "../../types.h"

namespace data {
namespace intermediate {

	struct Mesh {
		uint32 indexSize;
		core::Bytes vertices,indices;

		Mesh();
	};
	struct Material {
		vec3f diffuse,specular,emmisive,ambient;
		float shininess;

		enum { kTextureDiffuse,kTextureNormal,kTextureSpecular,kMaxTextures = 16 };
		const char* textureFiles[kMaxTextures];

		void release(core::Allocator* allocator);
	};

namespace mesh {
	class Reader {
	public:
		//Useful for fine grained import control
		struct Options {
			bool optimize;
			bool leftHanded;
			uint32 maxBonesPerVertex;
			enum VertexFormat {
				Position,
				PositionNormal,
				PositionNormalTexcoord0,
				PositionNormalTangentTexcoord0,
				PositionNormalTangentBitangentTexcoord0,

				VertexWeights = 0x8000,
			};

			inline Options() : optimize(false),leftHanded(false),maxBonesPerVertex(4) {}
		};

		void load(core::Allocator* allocator,const char* name,const Options& options = Options());

		virtual void processMesh(const Mesh& mesh,uint32 vertexFormat,const Material* material = nullptr) = 0;
		virtual void processSkeleton(uint32 boneCount,const Bone* bones);
		virtual void processSkeletalAnimation(const char* name,const animation::Animation& animation);
	};
} } }