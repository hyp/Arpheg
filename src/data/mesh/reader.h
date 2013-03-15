#pragma once

#include "../../core/memoryTypes.h"
#include "../../core/math.h"
#include "../../core/bytes.h"

namespace data {
namespace intermediate {

	struct Bone {
		mat44f offsetMatrix;
		mat44f transformMatrix;
		uint32 parent;
	};
	namespace animation {
		struct PositionKey {
			float time;
			vec3f position;
		};
		struct RotationKey {
			float time;
			Quaternion rotation;
		};
		struct Track {
			uint32 boneId;

			uint32 positionKeyCount;
			PositionKey* positionKeys;
			uint32 rotationKeyCount;
			RotationKey* rotationKeys;
		};
	}
	struct Mesh {
		uint32 indexSize;
		core::Bytes vertices,indices,bones;

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
				PositionNormalTangentBitangentTexcoord0
			};

			inline Options() : optimize(false),leftHanded(false),maxBonesPerVertex(4) {}
		};

		void load(core::Allocator* allocator,const char* name,const Options& options = Options());

		virtual bool processMesh(const Mesh& mesh,const Material* material = nullptr) = 0;
		virtual void processSkeletalAnimationTrack(const animation::Track& track);
	};
} } }