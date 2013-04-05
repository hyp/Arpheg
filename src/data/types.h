#pragma once

#include "../core/memoryTypes.h"
#include "../core/bytes.h"
#include "../rendering/types.h"

namespace data {

	class Service;
	
	typedef const char* ID;
	typedef uint32 BundleID;

	struct Pipeline;
	struct Image;
	struct Sprite;
	struct Font;

#ifndef ARPHEG_DATA_NO3D
	
	struct Material;
	struct SubMesh;

#endif
	
	//A pipeline is TODO
	struct Pipeline {
		typedef rendering::Pipeline::Constant Constant;
		
		Pipeline(rendering::Pipeline pipeline);

		inline rendering::Pipeline pipeline() const;
		inline rendering::Pipeline::Constant& modelViewProjectionConstant();
	private:
		rendering::Pipeline pipeline_;
		rendering::Pipeline::Constant mvp;
	};
	inline rendering::Pipeline Pipeline::pipeline() const { return pipeline_; }
	inline rendering::Pipeline::Constant& Pipeline::modelViewProjectionConstant() { return mvp; }

	namespace normalizedUint16 {
		typedef uint16 Type;
		enum { one = 0xFFFF };

		static inline Type make(float x){
			return Type(x * float(one));
		}
	};

	// A sprite is an image or a series of images residing on a single texture.
	struct Sprite {
		struct Frame {
			typedef normalizedUint16::Type TextureCoordinate;

			TextureCoordinate textureCoords[4];

			inline Frame() {}
			explicit Frame(float* texcoords);
		};

		inline Sprite() {}
		explicit Sprite(vec2i size);
		inline rendering::Texture2D texture() const;
		inline vec2i size() const;
		inline uint32 frameCount() const;
		inline Frame* frames() const;
	
		rendering::Texture2D texture_;
		uint32  size_;
		uint32  frameCount_;
		Frame*  frames_;
	};
	inline rendering::Texture2D Sprite::texture() const { return texture_; }
	inline vec2i Sprite::size() const { return vec2i( int32(size_&0xFFFF),int32(size_>>16) ); }
	inline uint32 Sprite::frameCount() const { return frameCount_; }
	inline Sprite::Frame* Sprite::frames() const { return frames_; }

#ifndef ARPHEG_DATA_NO3D
	
	//Material defines how a mesh is rendered
	struct Material {
		enum { kConstantsVec4fCount = 4 };
		enum { kMaxTextures = 5 };
		enum { kConstantsSize = kConstantsVec4fCount*sizeof(vec4f) };

		inline Material() {}
		Material(const rendering::Texture2D* textures,size_t textureCount,const void* constants,size_t constantsSize);

		inline uint32 textureCount() const;
		inline const rendering::Texture2D* textures() const;
		//Shader constants.
		inline core::Bytes constants();	
		inline vec4f* vec4fConstants();
	private:
		uint32 data_;
		rendering::Texture2D textures_[kMaxTextures];
		vec4f parameterStorage_       [kConstantsVec4fCount];
	};
	inline uint32 Material::textureCount() const { return data_; }
	inline const rendering::Texture2D* Material::textures() const { return textures_; }
	inline core::Bytes Material::constants() {
		return core::Bytes(parameterStorage_,kConstantsSize);
	}
	inline vec4f* Material::vec4fConstants() {
		return parameterStorage_;
	}

	typedef mat34fRowMajor Transformation3D;

	// A submesh is a renderable mesh resource.
	struct SubMesh {
		//A joint is used for skeletal animation (it stores the bind pose matrix).
		typedef mat44f Joint;

		SubMesh(const rendering::Mesh& mesh,uint32 offset,uint32 count,uint32 indexSize,rendering::topology::Primitive mode);
		inline SubMesh() { }

		inline const rendering::Mesh& mesh() const;
		inline uint32 primitiveCount() const;
		inline uint32 primitiveOffset() const;
		inline uint32 indexSize() const;
		inline rendering::topology::Primitive primitiveKind() const;
		inline Material* material() const;
		inline size_t skeletonJointsBegin() const;
		inline Joint* skeletonJoints() const;
		
		Material* material_;
		rendering::Mesh mesh_;
		uint32 data_;
		uint32 primitiveOffset_;
		Joint* skeletonJoints_;
	private: 
		enum { kCountMask = 0xFFFFFF };
		enum { kIndexOffset = 24,kIndexSizeMask = 0xF };
		enum { kPrimOffset = 28 };
	};
	inline Material* SubMesh::material() const { return material_; }
	inline const rendering::Mesh& SubMesh::mesh() const { return mesh_; }
	inline uint32 SubMesh::primitiveCount()  const{ return data_ & kCountMask; }
	inline uint32 SubMesh::primitiveOffset() const{ return primitiveOffset_; }
	inline uint32 SubMesh::indexSize()       const{ return (data_ >> kIndexOffset) & kIndexSizeMask; }
	inline rendering::topology::Primitive SubMesh::primitiveKind()  const{ 
		return rendering::topology::Primitive(data_ >> kPrimOffset); 
	}
	inline SubMesh::Joint* SubMesh::skeletonJoints() const {
		return skeletonJoints_;
	}
	inline size_t SubMesh::skeletonJointsBegin() const {
		return 0;
	}

	//A mesh is a collection of submeshes, it also can have a skeleton
	class Mesh {
	public:
		typedef uint16 SkeletonJointId;
		enum { kMaxSkeletonNodes = 0xFFFF };
		enum { kMaxBones = 128 };

		explicit Mesh(SubMesh* singleSubMesh);
		Mesh(SubMesh** submeshes,size_t count);
		inline size_t submeshCount() const;
		inline SubMesh* submesh(size_t i = 0) const;
		inline bool hasSkeleton() const;

		inline size_t skeletonNodeCount() const;
		inline SkeletonJointId* skeletonHierarchy() const;
		inline Transformation3D* skeletonDefaultLocalTransformations() const;
		
		union SubmeshArray {
			SubMesh* oneSubmesh;
			SubMesh** manySubMeshes;
		};
		uint32 submeshCount_;
		uint32 boneCount_;
		SubmeshArray submeshes_;

		SkeletonJointId* skeletonHierarchy_;
		Transformation3D* skeletonLocalTransforms_;
	};
	inline size_t Mesh::submeshCount() const {
		return size_t(submeshCount_);
	}
	inline SubMesh* Mesh::submesh(size_t i) const {
		return submeshCount_ < 2? submeshes_.oneSubmesh : submeshes_.manySubMeshes[i];
	}
	inline bool Mesh::hasSkeleton() const {
		return boneCount_!=0;
	}
	inline size_t Mesh::skeletonNodeCount() const {
		return size_t(boneCount_);
	}
	inline Mesh::SkeletonJointId* Mesh::skeletonHierarchy() const { return skeletonHierarchy_; } 
	inline Transformation3D* Mesh::skeletonDefaultLocalTransformations() const { return skeletonLocalTransforms_; } 

	//Animation
	namespace animation {
		namespace packing {
			// Packs the bone weight and id into one float value
			inline float boneWeightAndIdToFloat(float weight,int id){
				return weight*0.5f + float(id);
			}
		}

		//Keys
		struct PositionKey {
			float time;
			vec3f position;
		};
		struct RotationKey {
			float time;
			Quaternion rotation;
		};
		typedef PositionKey ScalingKey;
		
		//A track affects a single node.
		struct Track {
			uint32 nodeId;
			uint32 positionKeyCount;
			uint32 rotationKeyCount;
			uint32 scalingKeyCount;
			PositionKey* positionKeys;
			RotationKey* rotationKeys;
			ScalingKey*  scalingKeys;
		};
		
		//An animation consists of multiple tracks.
		struct Animation {
			uint32 trackCount;
			float  length,frequency;
			Track* tracks;
		};
	}

#endif

}