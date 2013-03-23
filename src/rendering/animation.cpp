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
NodeTransformationBuffer::Transformation* NodeTransformationBuffer::allocate(size_t nodeCount){
	auto thread = services::tasking()->threadId();
	assert(thread < threadStorageCount_);
	return (NodeTransformationBuffer::Transformation*)threadStorage_[thread].allocate(sizeof(Transformation)*nodeCount,alignof(Transformation));
}

void Player::animate(const data::animation::Animation* animation,size_t boneCount,const data::Bone* bones,mat44f* final,float& time,const mat44f* bindPose){

	float timeInTicks = time * animation->frequency;
	float localTime = fmod(timeInTicks,animation->length);

	for(size_t i = 0;i<boneCount;++i){
		final[i] = bones[i].transform;
	}

	//Calculate the interpolated local transform from the animation for relevant nodes
	for(size_t i = 0;i<size_t(animation->trackCount);++i){
		auto track = animation->tracks + i;
		//float localTime = fmod(time,track->length);
		size_t firstKey;
		vec3f position = vec3f(0,0,0);
		Quaternion rotation = Quaternion::identity();
		vec3f scaling = vec3f(1,1,1);
		//Find the position track.
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
				rotation = Quaternion(interpolate(track->rotationKeys[firstKey],track->rotationKeys[secondKey],localTime).v().normalize());
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

		final[track->nodeId] =  ( mat44f::translate(position) * mat44f::scale(scaling) * mat44f::rotate(rotation) ) ;//* final[track->nodeId] ;// * 
	}

	//Compute the world transformation for each node.
	for(size_t i = 1;i<boneCount;++i){
		//This verifies that the bones are sorted and the hirearchy is processed correctly
		assert(bones[i].parentId < i);
		final[i] =  final[bones[i].parentId] * final[i];
	}

	//Calculate the final matrix taking bind pose into account.
	//bindPose = &bones[0].offset;
	if(bindPose){
		for(size_t i = 1;i<boneCount;++i){
			final[i] = final[i] * (bones[i].offset * (*bindPose)) ;////(final[i] * bones[i].offset) * (*bindPose);//( ((*bindPose) * bones[i].offset) * final[i];
		}
	} else {
		//assert(false);
		for(size_t i = 1;i<boneCount;++i){
			final[i] = final[i] * bones[i].offset;
		}
	}
	//time = fmod(timeInTicks,animation->length*animation->frequency);
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