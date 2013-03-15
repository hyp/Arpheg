#pragma once

#include "../core/math.h"
#include "../core/memoryTypes.h"
#include "../data/types.h"

namespace rendering {
namespace animation {

//Stores the transformations for the bones and sends them to the GPU
class NodeTransformationBuffer {
public:
	//TODO: Can be replaced by 4x3 matrix!
	typedef mat44f Transformation;
	
	struct Index {
		uint32 offset;
		uint32 length;
	};
	
	Transformation* allocate(size_t nodeCount);
	void upload();
	inline Buffer buffer() const;
	
	NodeTransformationBuffer(core::Allocator* allocator);
	~NodeTransformationBuffer();
	void servicePreStep();
	
private:	
	//The gpu constant buffer
	Buffer constantBuffer_;
	typedef core::BufferAllocator Allocator;
	size_t threadStorageCount_;
	Allocator* threadStorage_;
	
	//Non copyable
	inline NodeTransformationBuffer(const NodeTransformationBuffer& other) {}
	inline void operator = (const NodeTransformationBuffer& other) {}
};
inline Buffer NodeTransformationBuffer::buffer() const {
	return constantBuffer_;
}

//Plays the animation
class Player {
public:
	//Key frame interpolation.
	static inline vec3f interpolate(const data::animation::PositionKey& a,const data::animation::PositionKey& b,float time){
		//Lerp
		return a.position + (b.position - a.position) * ((time - a.time) / (b.time - a.time));
	}
	static inline Quaternion interpolate(const data::animation::RotationKey& a,const data::animation::RotationKey& b,float time){
		//Lerp
		float k = ((time - a.time) / (b.time - a.time));
		return Quaternion::lerp(a.rotation,b.rotation,k);
	}

	static void animate(const data::animation::Animation* animation,size_t nodeCount,const data::Bone* bones,mat44f* final,float& time,const mat44f* bindPose = nullptr);
};

//Manages the whole animation system
class Service {
public:
	Service(core::Allocator* allocator);
	~Service();
	void servicePreStep();
	inline NodeTransformationBuffer& transformationBuffer();
	
private:
	NodeTransformationBuffer transformationBuffer_;
};
inline NodeTransformationBuffer& Service::transformationBuffer()  { return transformationBuffer_; }

} }