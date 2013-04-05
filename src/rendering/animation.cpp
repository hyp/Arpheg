#include "../core/assert.h"
#include "../core/bufferStringStream.h"
#include "../services.h"
#include "../application/tasking.h"
#include "animation.h"

namespace rendering {
namespace animation {

NodeTransformationBuffer::NodeTransformationBuffer(core::Allocator* allocator){
	constantBuffer_ = Buffer::nullBuffer();
	
	threadStorageCount_ = services::tasking()->threadCount();
	threadStorage_ = (Allocator*)allocator->allocate(sizeof(Allocator)*threadStorageCount_,alignof(Allocator));
	//Allocate storage for final matrices for each thread.
	//TODO locked backing allocator.
	//TODO use a chain allocator?
	for(size_t i = 0;i< threadStorageCount_;++i){
		new(threadStorage_ + i) core::BufferAllocator(8192,core::memory::globalAllocator(),core::BufferAllocator::GrowOnOverflow);
	}
}
NodeTransformationBuffer::~NodeTransformationBuffer(){
	//Release the storage allocators
	for(size_t i = 0; i < threadStorageCount_;++i){
		threadStorage_[i].~BufferAllocator();
	}
	//Release the GPU buffers
	if(!constantBuffer_.isNull()){
		auto renderer = services::rendering();
		renderer->release(constantBuffer_);
	}
}
void NodeTransformationBuffer::servicePreStep() {
	//Release the final matrices from the last frame.
	for(size_t i = 0; i < threadStorageCount_;++i){
		threadStorage_[i].reset();
	}
}
void NodeTransformationBuffer::upload() {
	size_t totalMemory = 0;
	for(size_t i = 0;i < threadStorageCount_;++i){
		totalMemory += threadStorage_[i].size();
	}
	if(!totalMemory) return;
	
	auto renderer = services::rendering();
	if(constantBuffer_.isNull()) {
		constantBuffer_  = renderer->create(rendering::Buffer::Constant,true,totalMemory);
	} else {
		renderer->recreate(rendering::Buffer::Constant,constantBuffer_,true,totalMemory);
	}
	auto mapping = renderer->map(rendering::Buffer::Constant,constantBuffer_);
	uint8* ptr = (uint8*) mapping.data;
	for(size_t i = 0;i < threadStorageCount_;++i){
		auto size = threadStorage_[i].size();
		if(size){
			memcpy(ptr,threadStorage_[i].bufferBase(),size);
			ptr += size;
		}
	}
	renderer->unmap(mapping);
}
Transformation3D* NodeTransformationBuffer::allocate(size_t nodeCount){
	auto thread = services::tasking()->threadId();
	assert(thread < threadStorageCount_);
	return (Transformation3D*)threadStorage_[thread].allocate(sizeof(Transformation3D)*nodeCount,alignof(Transformation3D));
}

struct TouchedNodeMask {
	enum { kSizeofSizetInBits = sizeof(size_t)*8 };
	size_t bits[Animator::kMaxNodes/kSizeofSizetInBits];

	TouchedNodeMask(){
		for(size_t i = 0;i<Animator::kMaxNodes/kSizeofSizetInBits;++i) bits[i] = 0;
	}
	inline void mark(size_t i){
		size_t element= i/kSizeofSizetInBits;
		size_t bit    = i%kSizeofSizetInBits;
		bits[element] |= 1<<bit;
	}
	inline bool isNotTouched(size_t i){
		size_t element= i/kSizeofSizetInBits;
		size_t bit    = i%kSizeofSizetInBits;
		return (bits[element] & (1<<bit)) == 0;
	}
};

//Obtain and store the wrapped local time
inline float Animator::calculateLocalTime(const data::animation::Animation* animation,float& time) {
	auto localTime = fmod(time * animation->frequency,animation->length);
	time = localTime / animation->frequency;
	return localTime;
}
//Calculate the interpolated local transform for a given track.
inline Transformation3D Animator::calculateLocalTransformation(const data::animation::Track* track,float localTime) {
	size_t firstKey;
	vec3f position = vec3f(0,0,0);
	Quaternion rotation = Quaternion::identity();
	vec3f scaling = vec3f(1,1,1);

	for(firstKey = 0;firstKey<track->positionKeyCount-1;++firstKey){
		if(localTime < track->positionKeys[firstKey+1].time){
			auto secondKey = (firstKey + 1)!=track->positionKeyCount? firstKey+1 : size_t(0);
			position = interpolate(track->positionKeys[firstKey],track->positionKeys[secondKey],localTime);
			break;
		}
	}

	for(firstKey = 0;firstKey<track->rotationKeyCount-1;++firstKey){
		if(localTime < track->rotationKeys[firstKey+1].time){
			auto secondKey = (firstKey + 1)!=track->rotationKeyCount? firstKey+1 : size_t(0);
			rotation = interpolate(track->rotationKeys[firstKey],track->rotationKeys[secondKey],localTime);
			break;
		}
	}

	for(firstKey = 0;firstKey<track->scalingKeyCount-1;++firstKey){
		if(localTime < track->scalingKeys[firstKey+1].time){
			auto secondKey = (firstKey + 1)!=track->scalingKeyCount? firstKey+1 : size_t(0);
			scaling = interpolate(track->scalingKeys[firstKey],track->scalingKeys[secondKey],localTime);
			break;
		}
	}
	return Transformation3D::translateRotateScale(position,rotation,scaling);
}

void Animator::animate(const data::Mesh* mesh,const data::animation::Animation* animation,float& time,Transformation3D* transformations) {
	size_t nodeCount = mesh->skeletonNodeCount();
	if(!nodeCount) return; 
	assert(nodeCount < kMaxNodes);

	TouchedNodeMask touchedNodesMask;
	auto localTime = calculateLocalTime(animation,time);

	//Local transformation.
	for(size_t i = 0;i<size_t(animation->trackCount);++i){
		auto track = animation->tracks + i;
		transformations[track->nodeId] = calculateLocalTransformation(track,localTime);
		touchedNodesMask.mark(size_t(track->nodeId));
	}

	//Global transformation.
	auto parentIds = mesh->skeletonHierarchy();
	auto defaults  = mesh->skeletonDefaultLocalTransformations();
	if(touchedNodesMask.isNotTouched(0))
		transformations[0] = defaults[0];
	for(size_t i = 1;i<nodeCount;++i){
		//transformations[i] = defaults[i] * transformations[parentIds[i]]
		if(touchedNodesMask.isNotTouched(i)) Transformation3D::multiply(defaults[i],transformations[parentIds[i]],transformations[i]);
		//transformations[i] *= transformations[parentIds[i]]
		else Transformation3D::multiply(transformations[i],transformations[parentIds[i]]);
	}
}
void Animator::setDefaultSkeleton(const data::Mesh* mesh,Transformation3D* transformations) {
	size_t nodeCount = mesh->skeletonNodeCount();
	if(!nodeCount) return; 
	assert(nodeCount < kMaxNodes);
	auto parentIds = mesh->skeletonHierarchy();
	auto defaults  = mesh->skeletonDefaultLocalTransformations();
	transformations[0] = defaults[0];
	for(size_t i = 1;i<nodeCount;++i){
		transformations[i] = defaults[i] * transformations[parentIds[i]];

	}
}
void Animator::bindSkeleton(const data::SubMesh* submesh,size_t nodeCount,const Transformation3D* skeletonTransformations,JointTransformation3D* jointTransformations) {
	auto bindPoses = submesh->skeletonJoints();
	for(size_t i = 1;i<nodeCount;++i){
		jointTransformations[i] = mat44f(skeletonTransformations[i]) * bindPoses[i];
	}
}

Service::Service(core::Allocator* allocator) 
	: transformationBuffer_(allocator) {
}
Service::~Service(){
}
void Service::servicePreStep(){
	transformationBuffer().servicePreStep();
}

} } 