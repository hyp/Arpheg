#include <limits>
#include "../../core/types.h"

#if !defined(ARPHEG_RESOURCES_NO_FORMATTED) && !defined(ARPHEG_PLATFORM_MOBILE)

#include "../../core/assert.h"
#include "../../core/io.h"
#include "../../core/memory.h"
#include "../../core/math.h"
#include "../../core/allocatorNew.h"
#include "../../core/bufferArray.h"
#include "../../core/bufferStringStream.h"
#include "../../services.h"
#include "../data.h"
#include "reader.h"

#include "../../dependencies/assimp/include/assimp/Importer.hpp"      // C++ importer interface
#include "../../dependencies/assimp/include/assimp/scene.h"
#include "../../dependencies/assimp/include/assimp/postprocess.h"

namespace data {
namespace intermediate {

void Material::release(core::Allocator* allocator){
	for(uint32 i = 0; i <kMaxTextures;++i){
		if(textureFiles[i]) allocator->deallocate((void*)textureFiles[i]);
	}
}
Mesh::Mesh() : vertices(nullptr,nullptr),indices(nullptr,nullptr),bones(nullptr,nullptr) {
	indexSize = 0;
}

namespace mesh {

inline void move3(float* dest,const aiVector3D& v){
	dest[0] = v.x;dest[1] = v.y;dest[2] = v.z;
}

static void convertMatrix(mat44f* dest,const aiMatrix4x4& matrix){
	float* f = &dest->a.x;
	for(uint32 i = 0;i<16;++i){
		f[i] = *matrix[i];
	}
}

struct BoneNode {
	uint32 id;
	const aiNode* parent;
	const aiNode* self;
	aiMatrix4x4 transform;
};

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
	inline void* allocateBones(size_t size){
		dest->bones.begin = (uint8*)allocator->allocate(size,alignof(vec4f));
		dest->bones.end = dest->bones.begin + size;
		return dest->bones.begin;
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
	void clearVertexWeights(aiMesh* mesh){
		float* vs = (float*)( dest->vertices.begin + vertexWeightOffset);
		for(uint32 i =0;i < mesh->mNumVertices;++i,vs = (float*) (((uint8*)vs)+vertexStride) ){
			for(uint32 j = 0;j< weightsPerVertex;++j) vs[j] = 0.0f;
		}
	}
	void setVertexWeight(uint32 vertexId,uint32 weightId,float weight,int boneId){
		auto ptr = dest->vertices.begin + vertexStride*vertexId + vertexWeightOffset;
		((float*)ptr)[weightId] = ::data::animation::packing::boneWeightAndIdToFloat(weight,boneId);
	}

	size_t bonesNodesCount;
	BoneNode* boneNodes;

	BoneNode* findNode(const aiBone* bone){
		for(size_t i = 0;i<bonesNodesCount;++i){
			if(boneNodes[i].self->mName == bone->mName) return boneNodes + i;
		}
		return nullptr;
	}
	uint32 findParentId(aiMesh* mesh,const aiNode* parent){
		if(!parent) return 0xFFFF;//Root
		for(uint32 i = 0;i <mesh->mNumBones;++i){
			auto bone = mesh->mBones[i];
			auto node = findNode(bone);
			if(!node) continue;
			if(node->self == parent) return i;
		}
		assertRelease(false && "Bone's parent not found!");
		return 0;
	}
	void importBones(aiMesh* mesh){
		if(!mesh->HasBones() || (vertexWeightOffset == vertexStride) ){
			dest->bones = core::Bytes(nullptr,nullptr);
			return;
		}
		clearVertexWeights(mesh);
		//Allocate weights info
		uint8* weightInfo = (uint8*)allocator->allocate(mesh->mNumVertices);
		memset(weightInfo,0,mesh->mNumVertices);
		assertRelease(dest->vertices.begin);
		
		//Prepare the bones
		for(uint32 i = 0;i <mesh->mNumBones;++i){
			auto bone = mesh->mBones[i];
			auto node = findNode(bone);
			if(!node){
				services::logging()->warning("A bone doesn't have a skeleton node attached!");
				continue;
			}
			node->id = i;
		}
		
		Bone* destBones = (Bone*)allocateBones(sizeof(Bone)*mesh->mNumBones);

		for(uint32 i = 0;i <mesh->mNumBones;++i){
			auto bone = mesh->mBones[i];
			auto node = findNode(bone);
			if(!node) continue;
			
			for(uint32 j = 0;j < bone->mNumWeights;++j){
				auto id     = bone->mWeights[j].mVertexId;
				auto weight = bone->mWeights[j].mWeight;
				if(weightInfo[id] >= weightsPerVertex) continue;
				setVertexWeight(id,weightInfo[id],weight,int(i));
				weightInfo[id]++;
			}
			destBones[i].parent = findParentId(mesh,node->parent);
			convertMatrix(&destBones[i].offsetMatrix,bone->mOffsetMatrix);
			convertMatrix(&destBones[i].transformMatrix,node->transform);
		}

		allocator->deallocate(weightInfo);
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
	core::BufferAllocator boneNodes;

	const aiNode* skeletonRoot;
	const aiNode* singleNode;
	aiMatrix4x4 singleNodeTransformInverse;
	
	uint32 indexOffset;
	size_t mergedVertexBufferSize;
	size_t mergedIndexBufferSize;
	uint8 storageForImporter[MeshImporter::kMaxSizeof];
	Material mat;

	Scene(core::Allocator* allocator);
	Mesh importIntoOneMesh(const aiScene* scene);
	void recursiveImportIntoOneMesh(aiNode* node,aiMatrix4x4 transform);
	void recursiveImportBoneNode(const aiNode* node,const aiNode* parent,aiMatrix4x4 transform);

	void selectVertexFormat(const aiMesh* mesh);
	void importMaterial(const aiMaterial* material);
	void findSkeleton();
	inline bool hasSkeleton() const { return skeletonRoot != nullptr; }
	void importSkeletalAnimationTracks(const aiMesh* mesh);
};
Scene::Scene(core::Allocator* allocator) : subMeshes(sizeof(Mesh)*64,allocator,core::BufferAllocator::GrowOnOverflow),
	boneNodes(sizeof(BoneNode)*128,allocator,core::BufferAllocator::GrowOnOverflow)
{
	importer = nullptr;
	this->allocator = allocator;
	scene = nullptr;
}
void Scene::findSkeleton() {
	
	struct Util {
		const aiScene* scene;
		Scene* importer;

		const std::pair<const aiNode*,int> findNodeRec(const aiNode* node,const aiBone* bone,int depth = 0){
			if(node->mName == bone->mName) return std::make_pair(node,depth);
			for(uint32 i = 0;i<node->mNumChildren;++i){
				auto found = findNodeRec(node->mChildren[i],bone,depth+1);
				if(found.first) return found;
			}
			return std::make_pair((const aiNode*)nullptr,0);
		}
		const std::pair<const aiNode*,int> findNode(const aiBone* bone){
			return findNodeRec(scene->mRootNode,bone);
		}


		void recursiveImportBoneNode(const aiNode* node,const aiNode* parent,aiMatrix4x4 transform) {
			//Node's transformation matrix
			auto matrix = transform*node->mTransformation;

			auto dest= core::bufferArray::allocate<BoneNode>(importer->boneNodes);
			dest->parent    = parent;
			dest->self      = node;
			dest->transform = node->mTransformation;//matrix;
			for (uint32 i = 0; i < node->mNumChildren; ++i) 
				recursiveImportBoneNode (node->mChildren[i],node,matrix);
		}
		void importBoneNode(const aiNode* node){
			recursiveImportBoneNode(node,nullptr,aiMatrix4x4());
		}
	};
	Util util;util.scene = scene;util.importer = this;

	const aiNode* rootNode = nullptr;
	int rootDepth = std::numeric_limits<int>::max();

	for(uint32 i = 0;i<scene->mNumMeshes;++i){
		auto mesh = scene->mMeshes[i];
		if(!mesh->HasBones()) continue;
		for(uint32 j = 0;j < mesh->mNumBones;++j){
			auto node = util.findNode(mesh->mBones[j]);
			if(node.first && node.second < rootDepth){
				rootNode = node.first;
				rootDepth = node.second;
			}
		}
	}
	skeletonRoot = rootNode;
	if(rootNode) util.importBoneNode(rootNode);
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
static const aiNodeAnim* findAnim(const aiAnimation* animation,const aiNode* node){
	for(uint32 i = 0;i < animation->mNumChannels;++i){
		if(animation->mChannels[i]->mNodeName == node->mName){
			return animation->mChannels[i];
		}
	}
	return nullptr;
}
void Scene::importSkeletalAnimationTracks(const aiMesh* mesh) {
	for(uint32 i = 0;i<scene->mNumAnimations;++i){
		//Foreach bone
		for(size_t j = 0;j<core::bufferArray::length<BoneNode>(boneNodes);++j){
			auto bone = core::bufferArray::begin<BoneNode>(boneNodes) + j;
			auto track = findAnim(scene->mAnimations[i],bone->self);
			if(!track) continue;
			if(track->mNumScalingKeys > 0){
				services::logging()->warning("The imported animation track has scaling keys! They will be ignored.");
			}
			animation::Track destTrack;
			destTrack.boneId = bone->id;
			if(track->mNumPositionKeys)
				destTrack.positionKeys = (animation::PositionKey*)allocator->allocate(sizeof(animation::PositionKey)*track->mNumPositionKeys,alignof(animation::PositionKey));
			else destTrack.positionKeys = nullptr;
			destTrack.positionKeyCount = track->mNumPositionKeys;
			if(track->mNumRotationKeys)
				destTrack.rotationKeys = (animation::RotationKey*)allocator->allocate(sizeof(animation::RotationKey)*track->mNumRotationKeys,alignof(animation::RotationKey));
			else destTrack.rotationKeys = nullptr;
			destTrack.rotationKeyCount = track->mNumRotationKeys;

			for(uint32 j = 0;j<track->mNumPositionKeys;++j){
				destTrack.positionKeys[j].time = track->mPositionKeys[j].mTime;
				destTrack.positionKeys[j].position = vec3f(track->mPositionKeys[j].mValue.x,track->mPositionKeys[j].mValue.y,track->mPositionKeys[j].mValue.z);
			}
			for(uint32 j = 0;j<track->mNumRotationKeys;++j){
				destTrack.rotationKeys[j].time = track->mRotationKeys[j].mTime;
				destTrack.rotationKeys[j].rotation = Quaternion(track->mRotationKeys[j].mValue.x,
					track->mRotationKeys[j].mValue.y,track->mRotationKeys[j].mValue.z,track->mRotationKeys[j].mValue.w);
			}

			reader->processSkeletalAnimationTrack(destTrack);
			if(destTrack.positionKeys) allocator->deallocate(destTrack.positionKeys);
			if(destTrack.rotationKeys) allocator->deallocate(destTrack.rotationKeys);
		}
	}
}
Mesh Scene::importIntoOneMesh(const aiScene* scene){
	singleNode = nullptr;
	skeletonRoot = nullptr;
	this->scene = scene;
	//Find(if any) animation skeletons and extract required boneNodes
	findSkeleton();
	//Select the vertex format.
	selectVertexFormat(scene->mMeshes[0]);
	if(hasSkeleton()){
		importer->boneNodes = core::bufferArray::begin<BoneNode>(boneNodes);
		importer->bonesNodesCount = core::bufferArray::length<BoneNode>(boneNodes);
	} else {
		importer->boneNodes = nullptr;
		importer->bonesNodesCount = 0;
	}
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
	//Import any skeletal animations
	if(scene->HasAnimations() && hasSkeleton()){
		if(!singleNode){
			services::logging()->resourceError("Can't import a scene as a single mesh with animation because it has multiple nodes","");
		}
		else importSkeletalAnimationTracks(scene->mMeshes[singleNode->mMeshes[0]]);
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

		if(singleNode){
			assertRelease(!result.bones.begin);
			result.bones = i->bones;
		} else {
			if(i->bones.begin) allocator->deallocate(i->bones.begin);
		}
	}
	return result;
}
void Scene::recursiveImportIntoOneMesh(aiNode* node,aiMatrix4x4 transform) {
	//Node's transformation matrix
	auto matrix = transform*node->mTransformation;
	
	if(node->mNumMeshes){
		if(!singleNode){
			singleNode= node;
			//singleNodeTransformInverse = matrix.Inverse();
		}
		else singleNode = nullptr;

		for(uint32 i = 0;i<node->mNumMeshes;++i){
			importer->dest = core::bufferArray::allocate<Mesh>(subMeshes);
			importer->importVertices(scene->mMeshes[node->mMeshes[i]],matrix);
			importer->importIndices (scene->mMeshes[node->mMeshes[i]],indexOffset);
			importer->importBones   (scene->mMeshes[node->mMeshes[i]]);
			indexOffset += scene->mMeshes[node->mMeshes[i]]->mNumVertices;

			mergedVertexBufferSize += importer->dest->vertices.length();
			mergedIndexBufferSize  += importer->dest->indices.length();
		}
	}

	for (uint32 i = 0; i < node->mNumChildren; ++i) 
		recursiveImportIntoOneMesh (node->mChildren[i],matrix);
}

void Reader::processSkeletalAnimationTrack(const animation::Track& track){
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
	if(processMesh(result,&doImport.mat)){
		allocator->deallocate(result.vertices.begin);
		allocator->deallocate(result.indices.begin);
		if(result.bones.begin)
			allocator->deallocate(result.bones.begin);
	}
}

} } }

#else

#include "reader.h"

void data::intermediate::mesh::Reader::load(const char* name,const Options& options) {
}

#endif
