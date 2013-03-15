#pragma once

#include "../core/memoryTypes.h"
#include "../core/bytes.h"
#include "../rendering/types.h"

namespace data {

	class Service;
	
	typedef const char* ID;

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
	private:
		rendering::Pipeline pipeline_;
	};
	inline rendering::Pipeline Pipeline::pipeline() const { return pipeline_; }

	// A sprite is an image or a series of images residing on a single texture.
	struct Sprite {
		struct Frame {
			vec2f texcoordMin,texcoordMax;
		};

		inline rendering::Texture2D texture() const;
		inline vec2f size() const;
		inline uint32 frameCount() const;
		inline Frame* frames() const;
	
		rendering::Texture2D texture_;
		vec2f  size_;
		uint32 frameCount_;
		Frame* frames_;
	};
	inline rendering::Texture2D Sprite::texture() const { return texture_; }
	inline vec2f Sprite::size() const { return size_; }
	inline uint32 Sprite::frameCount() const { return frameCount_; }
	inline Sprite::Frame* Sprite::frames() const { return frames_; }

#ifndef ARPHEG_DATA_NO3D
	
	//Material defines how a mesh is rendered
	struct Material {
	
		inline uint32 textureCount();
		inline rendering::Texture2D* textures();
		//inline re
	private:
		enum { kMaxTextures = 8 };
		uint32 textureCount_;
		rendering::Texture2D textures_[kMaxTextures];
		float shininess_;
	};
	inline uint32 Material::textureCount() { return textureCount_; }
	inline rendering::Texture2D* Material::textures() { return textures_; }

	//A bone is a joint in a skeleton
	struct Bone {
		uint32 parentId;
		mat44f offset;
	};

	// A submesh is a renderable mesh resource.
	struct SubMesh {
		SubMesh(const rendering::Mesh& mesh,uint32 offset,uint32 count,uint32 indexSize,rendering::topology::Primitive mode);
		inline SubMesh() { }

		inline rendering::Mesh& mesh();
		inline uint32 primitiveCount() const;
		inline uint32 primitiveOffset() const;
		inline uint32 indexSize() const;
		inline rendering::topology::Primitive primitiveKind() const;
		inline Material* material() const;
		inline uint32 skeletonBoneCount() const;
		inline Bone*  skeleton() const;
	private: 
		enum { kCountMask = 0xFFFFFF };
		enum { kIndexOffset = 24,kIndexSizeMask = 0xF };
		enum { kPrimOffset = 28 };
		Material* material_;
		rendering::Mesh mesh_;
		uint32 data_;
		uint32 primitiveOffset_;
	};
	inline Material* SubMesh::material() const { return material_; }
	inline rendering::Mesh& SubMesh::mesh() { return mesh_; }
	inline uint32 SubMesh::primitiveCount()  const{ return data_ & kCountMask; }
	inline uint32 SubMesh::primitiveOffset() const{ return primitiveOffset_; }
	inline uint32 SubMesh::indexSize()       const{ return (data_ >> kIndexOffset) & kIndexSizeMask; }
	inline rendering::topology::Primitive SubMesh::primitiveKind()  const{ 
		return rendering::topology::Primitive(data_ >> kPrimOffset); 
	}

	//A mesh is a collection of submeshes, it also can have a skeleton
	class Mesh {
	public:
		inline size_t submeshCount() const;
		inline SubMesh* submeshes() const;
		inline bool hasSkeleton() const;
		inline size_t skeletonBoneCount() const;
		inline Bone*  skeleton() const;
	
		uint32 submeshCount_;
		uint32 boneCount_;
		SubMesh* submeshes_;
		Bone* skeleton_;
	};
	inline size_t Mesh::submeshCount() const {
		return size_t(submeshCount_);
	}
	inline SubMesh* Mesh::submeshes() const {
		return submeshes_;
	}
	inline bool Mesh::hasSkeleton() const {
		return boneCount_!=0;
	}
	inline size_t Mesh::skeletonBoneCount() const {
		return size_t(boneCount_);
	}
	inline Bone*  Mesh::skeleton() const {
		return skeleton_;
	}

	//Animation
	namespace animation {
		namespace packing {
			// Packs the bone weight and id into one float value
			inline float boneWeightAndIdToFloat(float weight,int id){
				return weight*0.5f + float(id);
			}
		}

		//Bones
		struct PositionKey {
			float time;
			vec3f position;
		};
		struct RotationKey {
			float time;
			Quaternion rotation;
		};
		//Each node has individiual track
		struct Track {
			uint32 nodeId;
			uint32 positionKeyCount;
			uint32 rotationKeyCount;
			float  length;
			PositionKey* positionKeys;
			RotationKey* rotationKeys;
		};
		//An animation consists of bones and tracks.
		struct Animation {
			uint32 id;
			uint32 trackCount;
			float  length;
			Track* tracks;
		};
	}

#endif

}