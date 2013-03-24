// This module implements an importer for mesh files using the ASSIMP library.

#include <limits>
#include "../../core/types.h"

#if !defined(ARPHEG_RESOURCES_NO_FORMATTED) && !defined(ARPHEG_PLATFORM_MOBILE)

#include <map>
#include <algorithm>

#include "../../../core/assert.h"
#include "../../../core/io.h"
#include "../../../core/memory.h"
#include "../../../core/math.h"
#include "../../../core/allocatorNew.h"
#include "../../../core/bufferArray.h"
#include "../../../core/bufferStringStream.h"
#include "../../../services.h"
#include "../../data.h"
#include "reader.h"

// C++ importer interface
#include "../../../dependencies/assimp/include/assimp/Importer.hpp"      
#include "../../../dependencies/assimp/include/assimp/scene.h"
#include "../../../dependencies/assimp/include/assimp/postprocess.h"

namespace data {
namespace intermediate {

void Material::release(core::Allocator* allocator){
	for(uint32 i = 0; i <kMaxTextures;++i){
		if(textureFiles[i]) allocator->deallocate((void*)textureFiles[i]);
	}
}
Mesh::Mesh() : vertices(nullptr,nullptr),indices(nullptr,nullptr) {
	indexSize = 0;
}

namespace mesh {

inline void move3(float* dest,const aiVector3D& v){
	dest[0] = v.x;dest[1] = v.y;dest[2] = v.z;
}

static void convertMatrix(mat44f* dest,const aiMatrix4x4& matrix){
	//NB we need column major matrices so do transpose.
	*dest = mat44f(vec4f(matrix.a1,matrix.a2,matrix.a3,matrix.a4),vec4f(matrix.b1,matrix.b2,matrix.b3,matrix.b4),
		vec4f(matrix.c1,matrix.c2,matrix.c3,matrix.c4),vec4f(matrix.d1,matrix.d2,matrix.d3,matrix.d4));
	dest->transposeSelf();
}
static void convertMatrix2(mat44f* dest,const aiMatrix4x4& matrix){
	//NB we need column major matrices so do transpose.
	*dest = mat44f(vec4f(matrix.a1,matrix.a2,matrix.a3,matrix.a4),vec4f(matrix.b1,matrix.b2,matrix.b3,matrix.b4),
		vec4f(matrix.c1,matrix.c2,matrix.c3,matrix.c4),vec4f(matrix.d1,matrix.d2,matrix.d3,matrix.d4));
	//dest->transposeSelf();
}

struct Scene;


struct MeshImporter {
	enum { kMaxSizeof = 256 };

	Mesh* dest;
	size_t vertexStride, vertexWeightOffset;
	uint32 weightsPerVertex;
	core::Allocator* allocator;

	inline void* allocateVertices(size_t size){
		dest->vertices.begin = (uint8*)allocator->allocate(size*vertexStride,alignof(vec4f));
		dest->vertices.end   = dest->vertices.begin + size*vertexStride;
		return dest->vertices.begin;
	}
	inline void* allocateIndices(size_t size){
		dest->indices.begin = (uint8*)allocator->allocate(size,alignof(vec4f));
		dest->indices.end   = dest->indices.begin + size;
		return dest->indices.begin;
	}
	virtual void importVertices(aiMesh* mesh,aiMatrix4x4 matrix,aiQuaternion normalMatrix) = 0;
	virtual size_t vertexSize() const = 0;

	void importVertices(aiMesh* mesh,aiMatrix4x4 matrix){
		aiVector3D scaling,pos;
		aiQuaternion rotation;
		matrix.Decompose(scaling,rotation,pos);
		importVertices(mesh,matrix,rotation);
	}
	void importIndices(aiMesh* mesh) {
		if(mesh->mNumVertices <= std::numeric_limits<uint16>::max()) dest->indexSize = sizeof(uint16);
		else dest->indexSize = sizeof(uint32);

		if(dest->indexSize == sizeof(uint16)){
			uint16* indices = (uint16*)allocateIndices(sizeof(uint16)*mesh->mNumFaces*3);
			for(uint32 i = 0;i< mesh->mNumFaces;++i,indices+=3){
				indices[0] = mesh->mFaces[i].mIndices[0];
				indices[1] = mesh->mFaces[i].mIndices[1];
				indices[2] = mesh->mFaces[i].mIndices[2];
			}
		} else {
			uint32* indices = (uint32*)allocateIndices(sizeof(uint32)*mesh->mNumFaces*3);
			for(uint32 i = 0;i< mesh->mNumFaces;++i,indices+=3){
				indices[0] = mesh->mFaces[i].mIndices[0];
				indices[1] = mesh->mFaces[i].mIndices[1];
				indices[2] = mesh->mFaces[i].mIndices[2];
			}
		}
	}
	void importIndices(aiMesh* mesh,uint32 indexOffset) {
		dest->indexSize = sizeof(uint32);

		uint32* indices = (uint32*)allocateIndices(sizeof(uint32)*mesh->mNumFaces*3);
		for(uint32 i = 0;i< mesh->mNumFaces;++i,indices+=3){
			indices[0] = indexOffset+mesh->mFaces[i].mIndices[0];
			indices[1] = indexOffset+mesh->mFaces[i].mIndices[1];
			indices[2] = indexOffset+mesh->mFaces[i].mIndices[2];
		}
	}
	void clearVertexWeights(const aiMesh* mesh){
		assertRelease(dest->vertices.begin);

		float* vs = (float*)( dest->vertices.begin + vertexWeightOffset);
		for(uint32 i =0;i < mesh->mNumVertices;++i,vs = (float*) (((uint8*)vs)+vertexStride) ){
			for(uint32 j = 0;j< weightsPerVertex;++j) vs[j] = 0.0f;
		}
	}
	void setVertexWeight(uint32 vertexId,uint32 weightId,float weight,int boneId){
		auto ptr = dest->vertices.begin + vertexStride*vertexId + vertexWeightOffset;
		((float*)ptr)[weightId] = ::data::animation::packing::boneWeightAndIdToFloat(weight,boneId);
	}


};
struct PositionImporter:MeshImporter {
	size_t vertexSize() const {
		return sizeof(float)*3;
	}
	void importVertices(aiMesh* mesh,aiMatrix4x4 matrix,aiQuaternion normalMatrix) {
		auto vertices = mesh->mVertices;
		uint32 vcount = mesh->mNumVertices;
		float* dest = (float*)allocateVertices(vcount);
		for(uint32 i =0;i < vcount;++i,dest = (float*) (((uint8*)dest)+vertexStride) ){
			move3(dest,matrix*vertices[i]);
		}
	}
};
struct PositionNormalImporter:MeshImporter {
	size_t vertexSize() const {
		return sizeof(float)*6;
	}
	void importVertices(aiMesh* mesh,aiMatrix4x4 matrix,aiQuaternion normalMatrix) {
		auto vertices = mesh->mVertices;
		auto normals  = mesh->mNormals;
		uint32 vcount = mesh->mNumVertices;
		float* dest = (float*)allocateVertices(vcount);
		for(uint32 i =0;i < vcount;++i,dest = (float*) (((uint8*)dest)+vertexStride) ){
			move3(dest,matrix*vertices[i]);
			move3(dest+3,normalMatrix.Rotate(normals[i]));
		}
	}
};
struct PositionNormalTexcoord2DImporter:MeshImporter {
	size_t vertexSize() const {
		return sizeof(float)*8;
	}
	void importVertices(aiMesh* mesh,aiMatrix4x4 matrix,aiQuaternion normalMatrix){
		auto vertices  = mesh->mVertices;
		auto normals   = mesh->mNormals;
		auto texcoords = mesh->mTextureCoords[0];
		uint32 vcount  = mesh->mNumVertices;
		float* dest = (float*)allocateVertices(vcount);
		
		for(uint32 i =0;i < vcount;++i,dest = (float*) (((uint8*)dest)+vertexStride)){
			move3(dest,matrix*vertices[i]);
			move3(dest+3,normalMatrix.Rotate(normals[i]));
			dest[6] = texcoords[i].x;dest[7] = texcoords[i].y;
		}
	}
};



struct Scene {
	Reader* reader;
	Reader::Options options;
	const aiScene* scene;
	MeshImporter* importer;
	core::Allocator* allocator;
	core::BufferAllocator subMeshes;

	std::map<const aiBone*,uint32> skeletonBoneMapping;
	std::map<const aiNode*,uint32> skeletonNodeMapping;
	Bone* skeletonBones;
	size_t skeletonBoneCount;

	const aiNode* singleNode;
	aiMatrix4x4 singleNodeTransformInverse;
	
	uint32 indexOffset;
	size_t mergedVertexBufferSize;
	size_t mergedIndexBufferSize;
	uint8 storageForImporter[MeshImporter::kMaxSizeof];
	Material mat;

	::data::Mesh resultingMesh;

	Scene(core::Allocator* allocator);
	~Scene();
	Mesh importIntoOneMesh(const aiScene* scene);
	void recursiveImportIntoOneMesh(aiNode* node,aiMatrix4x4 transform);
	void recursiveImportBoneNode(const aiNode* node,const aiNode* parent,aiMatrix4x4 transform);

	void selectVertexFormat(const aiMesh* mesh);
	void importMaterial(const aiMaterial* material);
	void importMeshVertexWeights(const aiMesh* mesh);
	void findSkeleton();
	inline bool hasSkeleton() const { return skeletonBones != nullptr; }
	const aiNode* findSkeletonNode(const aiString& name);
	void importSkeletalAnimationTracks();
};
Scene::Scene(core::Allocator* allocator) : subMeshes(sizeof(Mesh)*64,allocator,core::BufferAllocator::GrowOnOverflow) {
	importer = nullptr;
	this->allocator = allocator;
	scene = nullptr;
	skeletonBones = nullptr;
}
Scene::~Scene() {
	if(skeletonBones) allocator->deallocate(skeletonBones);
}
void Scene::findSkeleton() {
	
	struct BoneNode {
		int depth;
		const aiBone* bone;
		const aiNode* self;
	};
	struct Util {
		const aiScene* scene;
		Scene* importer;
		const aiNode* skeletonRootNode;
		int skeletonRootDepth;
		bool skeletonNeedsRoot;
		aiMatrix4x4 skeletonRootTransform;
		core::BufferAllocator boneNodes;
		uint32 boneCount;

		Util() : boneNodes(sizeof(BoneNode)*128,nullptr,core::BufferAllocator::GrowOnOverflow) {}


		void recursiveImportBoneNode(const aiNode* node,const aiNode* parent,int depth = 0) {
			//Find the bone for this node.
			const aiBone* bone = nullptr;
			for(uint32 i = 0;i<scene->mNumMeshes;++i){
				auto mesh = scene->mMeshes[i];
				if(!mesh->HasBones()) continue;
				for(uint32 j = 0;j < mesh->mNumBones;++j){
					if(mesh->mBones[j]->mName == node->mName){
						bone = mesh->mBones[j];
					}
				}
			}

			auto dest= core::bufferArray::allocate<BoneNode>(boneNodes);
			dest->depth = depth;
			dest->bone = bone;
			dest->self = node;
			if(bone) boneCount++;
			if(bone){
				if(skeletonRootDepth > depth){
					skeletonRootNode = node;
					skeletonRootDepth = depth;
					skeletonNeedsRoot = false;
				} else if(skeletonRootDepth == depth){
					skeletonNeedsRoot = true;
				}
			}
		
			for (uint32 i = 0; i < node->mNumChildren; ++i) 
				recursiveImportBoneNode (node->mChildren[i],node,depth+1);
		}
		void sortBoneNodes(){
			using namespace core::bufferArray;
			//Sort the skeleton hierarchy.
			struct Predicate {
				bool operator ()(const BoneNode& a,const BoneNode& b){
					return a.depth < b.depth;
				}
			};
			std::sort(begin<BoneNode>(boneNodes),end<BoneNode>(boneNodes),Predicate());
		}
		aiMatrix4x4 globalTransform(const aiNode* node){
			return node?  node->mTransformation * globalTransform(node->mParent): aiMatrix4x4() ;
		}
		void importBoneNode(const aiNode* node){
			skeletonRootDepth = std::numeric_limits<int>::max();
			skeletonRootNode = nullptr;
			boneCount = 0;
			
			recursiveImportBoneNode(node,nullptr);
			if(boneCount){
				assert(skeletonRootNode);
				if(skeletonNeedsRoot) skeletonRootNode = skeletonRootNode->mParent;
				skeletonRootTransform = globalTransform(skeletonRootNode);
				skeletonRootTransform.Inverse();
			}
			sortBoneNodes();
		}
		BoneNode* findBoneNode(const aiBone* bone){
			auto begin = core::bufferArray::begin<BoneNode>(boneNodes);
			for(size_t i = 0;i<core::bufferArray::length<BoneNode>(boneNodes);++i){
				if(begin[i].bone == bone) return begin + i;
			}
			return nullptr;
		}
	};
	Util util;util.scene = scene;util.importer = this;

	util.importBoneNode(scene->mRootNode);
	skeletonBones = nullptr;
	if(!util.boneCount) return;
	auto totalBoneCount = core::bufferArray::length<BoneNode>(util.boneNodes);
	auto boneNodes = core::bufferArray::begin<BoneNode>(util.boneNodes);

	//Create the skeleton
	Bone* destBones = (Bone*)allocator->allocate(sizeof(Bone)*totalBoneCount,alignof(Bone));

	assertRelease(totalBoneCount <= ::data::Mesh::kMaxSkeletonNodes);
	auto destParentIds = (::data::Mesh::SkeletonJointId*) allocator->allocate(sizeof(::data::Mesh::SkeletonJointId)*totalBoneCount,alignof(::data::Mesh::SkeletonJointId));
	auto destLocalTransforms = (::data::Transformation3D*)allocator->allocate(sizeof(::data::Transformation3D)*totalBoneCount,alignof(::data::Transformation3D));
	resultingMesh.boneCount_ = totalBoneCount;
	resultingMesh.skeletonHierarchy_ =  destParentIds;
	resultingMesh.skeletonLocalTransforms_ = destLocalTransforms;

	//Convert the nodes.
	for(size_t i = 0;i<totalBoneCount;++i){
		//Compute the offset matrix.
		if(boneNodes[i].bone){
			convertMatrix(&destBones[i].offset,boneNodes[i].bone->mOffsetMatrix);
		}
		else destBones[i].offset = mat44f::identity();
		//Transformation matrix.
		convertMatrix(&destBones[i].transform,boneNodes[i].self->mTransformation);
		//Find parent id.
		uint32 parentId = 0;
		for(parentId = 0;parentId < i;parentId++){
			if(boneNodes[parentId].self == boneNodes[i].self->mParent) break;
		}
		if(parentId == i){
			assertRelease(i == 0 && "Only the root bone must be without parent!");
			destBones[i].parentId = 0;
		}
		else destBones[i].parentId = parentId;

		destParentIds[i] = ::data::Mesh::SkeletonJointId(parentId);
		convertMatrix(&destLocalTransforms[i],boneNodes[i].self->mTransformation);
		skeletonNodeMapping[boneNodes[i].self] = uint32(i);
		
		//Map the nodes and bones to ids.
		if(boneNodes[i].bone) skeletonBoneMapping[boneNodes[i].bone] = uint32(i);
	}

	//Verify that the bones are sorted hierachily.
	for(size_t i = 1;i <totalBoneCount;++i){
		assertRelease(destBones[i].parentId < i);
		assertRelease(destParentIds[i] < i);
	}

	//Extract the global bind pose.
	//auto m_GlobalInverseTransform = scene->mRootNode->mTransformation;
	//m_GlobalInverseTransform.Inverse();
	//convertMatrix(&destBones[0].offset,m_GlobalInverseTransform);//util.skeletonRootTransform);



	skeletonBones= destBones;
	skeletonBoneCount = totalBoneCount;
	
}
void Scene::importMaterial(const aiMaterial* material) {
	aiString name;
	bool named = AI_SUCCESS == material->Get(AI_MATKEY_NAME,name);
	
	aiColor3D diffuse,specular,emissive,ambient;
	float shininess,shininessStrength;
	bool hasDiffuse = AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE,diffuse);
	bool hasEmmisive = AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR,specular);
	bool hasSpecular = AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE,emissive);
	bool hasAmbient = AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT,ambient);
	bool hasShininess = AI_SUCCESS == material->Get(AI_MATKEY_SHININESS,shininess);
	bool hasShininessStrength = AI_SUCCESS == material->Get(AI_MATKEY_SHININESS_STRENGTH,shininessStrength);
	if(hasSpecular && hasShininessStrength){
		specular.r*=shininessStrength;specular.g*=shininessStrength;specular.b*=shininessStrength;
	}

	aiString diffuseTexture,normalTexture,specularTexture;
	
	bool hasDiffuseMap = AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE,0),diffuseTexture);
	bool hasNormalMap = AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS,0),normalTexture);
	bool hasSpecularMap = AI_SUCCESS == material->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR,0),specularTexture);

	auto logger = services::logging();
	if(logger->priority() <= application::logging::Trace){
		if(!named) named = "<unnamed>";
		using namespace core::bufferStringStream;
		
		enum { kBufferSize = 4096 };
		char buffer[kBufferSize];
		core::BufferAllocator fmt(core::Bytes(buffer,kBufferSize),core::BufferAllocator::ResetOnOverflow);
		
		printf(fmt,"Importing material '%s'\n",name.C_Str());
#define TRACE_TOSTR(val) #val
#define TRACE_COL3(has,val) if(has) printf(fmt,"  " TRACE_TOSTR(val) ": %f,%f,%f\n",val.r,val.g,val.b); else printf(fmt,"  " TRACE_TOSTR(val) ": n/a\n")
#define TRACE_STR(has,val)  if(has) printf(fmt,"  " TRACE_TOSTR(val) ": '%s'\n",val.C_Str()); else printf(fmt,"  " TRACE_TOSTR(val) ": n/a\n")
		TRACE_COL3(hasDiffuse,diffuse);
		TRACE_COL3(hasSpecular,specular);
		TRACE_COL3(hasAmbient,ambient);
		TRACE_COL3(hasEmmisive,emissive);
		if(hasShininess) printf(fmt,"  shininess: %f\n",shininess); else printf(fmt,"  shininess: n/a\n");

		TRACE_STR(hasDiffuseMap,diffuseTexture);
		TRACE_STR(hasNormalMap,normalTexture);
		TRACE_STR(hasSpecularMap,specularTexture);
		logger->trace(asCString(fmt));
#undef TRACE_STR
#undef TRACE_COL3
#undef TRACE_TOSTR
	}
	//mat.diffuse = diffuse;
	//mat.specular = specular;
	//mat.ambient = ambient;
	//mat.emissive = emissive;
	//mat.shininess = shininess;
	//if(hasDiffuseMap)
}
void Scene::selectVertexFormat(const aiMesh* mesh){
	auto logging = services::logging();
	if(!mesh->HasPositions()){
		logging->resourceError("A mesh has no vertex position data. It will be ignored",mesh->mName.C_Str());
		return;
	}
	if(mesh->GetNumColorChannels() > 0)
		logging->warning("A mesh has vertex colours which will be ignored!");

	bool normals   = mesh->HasNormals();
	bool texcoords = mesh->HasTextureCoords(0);

	if(normals){
		if(texcoords) importer = new(storageForImporter) PositionNormalTexcoord2DImporter;
		else importer = new(storageForImporter) PositionNormalImporter;
	} else importer = new(storageForImporter) PositionImporter;
	auto vertexSize = importer->vertexSize();
	importer->vertexWeightOffset = vertexSize;
	if(hasSkeleton()){ //has skeleton - add animation info
		importer->weightsPerVertex = options.maxBonesPerVertex;
		vertexSize+=sizeof(float)*importer->weightsPerVertex;
	}
	else importer->weightsPerVertex = 0;
	importer->vertexStride = vertexSize;
	assert(importer);
	importer->allocator = allocator;//NB: set allocator on creation field.
}

const aiNode* Scene::findSkeletonNode(const aiString& name) {
	for(auto i = skeletonNodeMapping.begin();i != skeletonNodeMapping.end();++i){
		if(i->first->mName == name) return i->first;
	}
	return nullptr;
}
template<typename T>
struct KeyPred {
	inline bool operator () (const T& a,const T& b){
		return a.time < b.time;
	}
};

void Scene::importSkeletalAnimationTracks() {
	for(uint32 i = 0;i<scene->mNumAnimations;++i){
		auto animation = scene->mAnimations[i];
		animation::Animation destAnimation;
		destAnimation.length = scene->mAnimations[i]->mDuration;
		destAnimation.frequency = scene->mAnimations[i]->mTicksPerSecond == 0.f? 25.f : scene->mAnimations[i]->mTicksPerSecond;
		destAnimation.trackCount = 0;
		destAnimation.tracks = nullptr;

		//Find the track count
		for(uint32 i = 0;i<animation->mNumChannels;++i){
			if(!findSkeletonNode(animation->mChannels[i]->mNodeName)){
				services::logging()->warning("An animation contains a track which doesn't influence any bone");
				continue;
			}
			destAnimation.trackCount++;
		}
		if(!destAnimation.trackCount) continue;
		destAnimation.tracks = (animation::Track*)allocator->allocate(sizeof(animation::Track)*destAnimation.trackCount,alignof(animation::Track));

		//Import the tracks.
		uint32 currentTrack = 0;
		for(uint32 i = 0;i<animation->mNumChannels;++i){
			auto track = animation->mChannels[i];
			auto node = findSkeletonNode(animation->mChannels[i]->mNodeName);
			if(!node) continue;
			if(track->mNumScalingKeys > 0){
				//services::logging()->warning("The imported animation track has scaling keys! They will be ignored.");
			}
			animation::Track& destTrack = destAnimation.tracks[currentTrack];

			destTrack.nodeId = skeletonNodeMapping[node];
			if(track->mNumPositionKeys)
				destTrack.positionKeys = (animation::PositionKey*)allocator->allocate(sizeof(animation::PositionKey)*track->mNumPositionKeys,alignof(animation::PositionKey));
			else destTrack.positionKeys = nullptr;
			destTrack.positionKeyCount = track->mNumPositionKeys;
			if(track->mNumRotationKeys)
				destTrack.rotationKeys = (animation::RotationKey*)allocator->allocate(sizeof(animation::RotationKey)*track->mNumRotationKeys,alignof(animation::RotationKey));
			else destTrack.rotationKeys = nullptr;
			destTrack.rotationKeyCount = track->mNumRotationKeys;
			if(track->mNumScalingKeys)
				destTrack.scalingKeys = (animation::ScalingKey*)allocator->allocate(sizeof(animation::ScalingKey)*track->mNumScalingKeys,alignof(animation::ScalingKey));
			else destTrack.scalingKeys = nullptr;
			destTrack.scalingKeyCount = track->mNumScalingKeys;

			for(uint32 j = 0;j<track->mNumPositionKeys;++j){
				destTrack.positionKeys[j].time = track->mPositionKeys[j].mTime;
				destTrack.positionKeys[j].position = vec3f(track->mPositionKeys[j].mValue.x,track->mPositionKeys[j].mValue.y,track->mPositionKeys[j].mValue.z);
			}
			std::sort(destTrack.positionKeys,destTrack.positionKeys+track->mNumPositionKeys,KeyPred<animation::PositionKey>());
			for(uint32 j = 0;j<track->mNumRotationKeys;++j){
				destTrack.rotationKeys[j].time = track->mRotationKeys[j].mTime;
				destTrack.rotationKeys[j].rotation = Quaternion(track->mRotationKeys[j].mValue.x,
					track->mRotationKeys[j].mValue.y,track->mRotationKeys[j].mValue.z,track->mRotationKeys[j].mValue.w);
			}
			std::sort(destTrack.rotationKeys,destTrack.rotationKeys+track->mNumRotationKeys,KeyPred<animation::RotationKey>());
			for(uint32 j = 0;j<track->mNumScalingKeys;++j){
				destTrack.scalingKeys[j].time = track->mScalingKeys[j].mTime;
				destTrack.scalingKeys[j].position = vec3f(track->mScalingKeys[j].mValue.x,track->mScalingKeys[j].mValue.y,track->mScalingKeys[j].mValue.z);
			}
			std::sort(destTrack.scalingKeys,destTrack.scalingKeys+track->mNumScalingKeys,KeyPred<animation::ScalingKey>());

			currentTrack++;
		}

		//Pass it along.
		reader->processSkeletalAnimation(animation->mName.C_Str(),destAnimation);

		//Release the memory
		for(uint32 i = 0;i < destAnimation.trackCount;++i){
			if(auto ptr = destAnimation.tracks[i].positionKeys) allocator->deallocate(ptr);
			if(auto ptr = destAnimation.tracks[i].rotationKeys) allocator->deallocate(ptr);
			if(auto ptr = destAnimation.tracks[i].rotationKeys) allocator->deallocate(ptr);
		}
		allocator->deallocate(destAnimation.tracks);	
	}
}
void Scene::importMeshVertexWeights(const aiMesh* mesh){
	if(!hasSkeleton()) return;

	importer->clearVertexWeights(mesh);
	
	//Allocate weights info
	uint8* vertexWeightCount = (uint8*)allocator->allocate(mesh->mNumVertices);
	memset(vertexWeightCount,0,mesh->mNumVertices);	

	for(uint32 i = 0;i <mesh->mNumBones;++i){
		auto bone = mesh->mBones[i];

		for(uint32 j = 0;j < bone->mNumWeights;++j){
			auto vertexId = bone->mWeights[j].mVertexId;
			auto weight = bone->mWeights[j].mWeight;
			assertRelease(weight>= 0.0f && weight <= 1.0f);
			if(vertexWeightCount[vertexId] >= options.maxBonesPerVertex){
				services::logging()->warning("A vertex has more bone weights than the limit allows");
				continue;
			}
			auto boneId = skeletonBoneMapping[bone];
			importer->setVertexWeight(vertexId,vertexWeightCount[vertexId],weight,int(boneId));
			vertexWeightCount[vertexId]++;
		}
	}

	allocator->deallocate(vertexWeightCount);
}
Mesh Scene::importIntoOneMesh(const aiScene* scene){
	singleNode = nullptr;
	skeletonBones = nullptr;
	this->scene = scene;
	//Find(if any) animation skeletons and extract required boneNodes
	findSkeleton();
	//Select the vertex format.
	selectVertexFormat(scene->mMeshes[0]);
	//Import the material
	uint32 materialCount = scene->mNumMaterials;
	if(materialCount > 1){
		services::logging()->resourceError("Can't import a mesh as a single mesh with more than one material","");
	} if(materialCount){
		importMaterial(scene->mMaterials[0]);
	}
	//Start from the 0th index
	indexOffset = 0;
	mergedVertexBufferSize = mergedIndexBufferSize = 0;
	//Import all submeshes
	recursiveImportIntoOneMesh(scene->mRootNode,aiMatrix4x4());
	if(singleNode && hasSkeleton()){
		//convertMatrix(&skeletonBones[0].offset,singleNodeTransformInverse);
	}
	//Import any skeletal animations
	if(scene->HasAnimations() && hasSkeleton()){
		if(!singleNode){
			services::logging()->resourceError("Can't import a scene as a single mesh with animation because it has multiple nodes","");
		}
		else importSkeletalAnimationTracks();
	}

	//Merge the submeshes into one mesh
	Mesh result;
	result.vertices.begin = (uint8*)allocator->allocate(mergedVertexBufferSize,alignof(vec4f));
	result.vertices.end   = result.vertices.begin + mergedVertexBufferSize;

	if(indexOffset <= std::numeric_limits<uint16>::max()){
		result.indexSize = sizeof(uint16);
		mergedIndexBufferSize /= 2;
	}
	else result.indexSize = sizeof(uint32);
	result.indices.begin = (uint8*)allocator->allocate(mergedIndexBufferSize,alignof(vec4f));
	result.indices.end   = result.indices.begin + mergedIndexBufferSize;

	auto vs  = result.vertices.begin;
	auto ids = result.indices.begin;
	for(Mesh* i = core::bufferArray::begin<Mesh>(subMeshes),*end = core::bufferArray::end<Mesh>(subMeshes);i<end;++i){
		
		//Copy vertices
		memcpy(vs,i->vertices.begin,i->vertices.length());
		vs+=i->vertices.length();
		allocator->deallocate(i->vertices.begin);

		//Copy indices
		if(result.indexSize == sizeof(uint32)){
			memcpy(ids,i->indices.begin,i->indices.length());
			ids+=i->indices.length();
		} else{
			for(uint32* j = (uint32*)i->indices.begin,*jend = (uint32*)(i->indices.end);j<jend;++j){
				*((uint16*)ids) = *j;
				ids+=sizeof(uint16);
			}
		}
		allocator->deallocate(i->indices.begin);

	}
	return result;
}
void Scene::recursiveImportIntoOneMesh(aiNode* node,aiMatrix4x4 transform) {
	//Node's transformation matrix
	auto matrix = aiMatrix4x4();//transform*node->mTransformation;
	
	if(node->mNumMeshes){
		if(!singleNode){
			singleNode= node;
			singleNodeTransformInverse = matrix;
			singleNodeTransformInverse.Inverse();
		}
		else singleNode = nullptr;

		for(uint32 i = 0;i<node->mNumMeshes;++i){
			importer->dest = core::bufferArray::allocate<Mesh>(subMeshes);
			importer->importVertices(scene->mMeshes[node->mMeshes[i]],matrix);
			importer->importIndices (scene->mMeshes[node->mMeshes[i]],indexOffset);
			importMeshVertexWeights(scene->mMeshes[node->mMeshes[i]]);
			indexOffset += scene->mMeshes[node->mMeshes[i]]->mNumVertices;

			mergedVertexBufferSize += importer->dest->vertices.length();
			mergedIndexBufferSize  += importer->dest->indices.length();
		}
	}

	for (uint32 i = 0; i < node->mNumChildren; ++i) 
		recursiveImportIntoOneMesh (node->mChildren[i],matrix);
}

void Reader::processSkeleton(uint32 boneCount,const Bone* bones) {
}
void Reader::processSkeletalAnimation(const char* name,const animation::Animation& track){
}
void Reader::load(core::Allocator* allocator,const char* name,const Options& options){
	Assimp::Importer importer;

	importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS,options.maxBonesPerVertex);
	//Request polygons only
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,aiPrimitiveType_LINE|aiPrimitiveType_POINT);
	//Flags
	uint32 flags = 	aiProcess_CalcTangentSpace 
		| aiProcess_GenNormals 
		| aiProcess_JoinIdenticalVertices 
		| aiProcess_Triangulate			
		| aiProcess_GenUVCoords           
		| aiProcess_SortByPType       //Request polygons only
		| aiProcess_TransformUVCoords 
		| aiProcess_LimitBoneWeights; //max N bone weights
	if(options.optimize){
		flags |= aiProcess_ImproveCacheLocality 
			| aiProcess_RemoveRedundantMaterials
			//| aiProcess_FindDegenerates
			| aiProcess_FindInvalidData;
	}
	if(options.leftHanded) flags |= aiProcess_ConvertToLeftHanded;

	auto scene = importer.ReadFile(name,flags);
	if(!scene){
		services::logging()->resourceError(importer.GetErrorString(),name);
		return;
	}
	auto root = scene->mRootNode;
	if(!scene->HasMeshes()){
		services::logging()->resourceError("No meshes present",name);
		return;
	}
	Scene doImport(allocator);doImport.reader = this;doImport.options = options;
	auto result = doImport.importIntoOneMesh(scene);
	processMesh(result,doImport.hasSkeleton()? Options::VertexWeights : 0,&doImport.mat);
	if(doImport.hasSkeleton())
		processSkeleton(doImport.skeletonBoneCount,doImport.skeletonBones);
	allocator->deallocate(result.vertices.begin);
	allocator->deallocate(result.indices.begin);
}

} } }

#else

#include "reader.h"

void data::intermediate::mesh::Reader::load(const char* name,const Options& options) {
}

#endif
