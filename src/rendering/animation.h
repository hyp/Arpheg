#pragma once

#include "../core/math.h"
#include "../core/memoryTypes.h"
#include "../data/types.h"

namespace rendering {
namespace animation {


typedef data::Transformation3D Transformation3D;
//Stores the final matrices which can be used by the gpu for skeletal animation.
typedef mat44f JointTransformation3D; 

//Stores the transformations for the bones and sends them to the GPU
class NodeTransformationBuffer {
public:
	struct Index {
		uint32 offset;
		uint32 length;
	};
	
	Transformation3D* allocate(size_t nodeCount);
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

//Animates the nodes by interpolating between keyframes
class Animator {
public:
	//Maximum amount of nodes in an animatable hierachy
	enum {
		kMaxNodes= 512,
	};

	//Key frame interpolation.
	static inline vec3f interpolate(const data::animation::PositionKey& a,const data::animation::PositionKey& b,float time){
		float k = (time - a.time) / (b.time - a.time);
		return a.position * (1.0f - k) + b.position * (k);
	}
	static inline Quaternion interpolate(const data::animation::RotationKey& a,const data::animation::RotationKey& b,float time){
		float k = ((time - a.time) / (b.time - a.time));
		return Quaternion::lerp(a.rotation,b.rotation,k);
	}

	static float  calculateLocalTime(const data::animation::Animation* animation,float& time);
	static Transformation3D calculateLocalTransformation(const data::animation::Track* track,float localTime);

	static void animate(const data::animation::Animation* animation,size_t nodeCount,const data::Bone* bones,mat44f* final,float& time,const mat44f* bindPose = nullptr);
	
	//Mesh animation
	static void animate     (const data::Mesh* mesh,const data::animation::Animation* animation,float& time,Transformation3D* skeletonTransformations);
	static void bindSkeleton(const data::SubMesh* submesh,size_t nodeCount,const Transformation3D* skeletonTransformations,JointTransformation3D* jointTransformations);
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