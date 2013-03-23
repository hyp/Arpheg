// Provides a thread safe service for debug rendering (wire boxes,lines, coordinate axises, etc)

#pragma once

#include "types.h"
#include "../core/thread/types.h"
#include "../core/math.h"
#include "../core/memory.h"
#include "../data/types.h"

namespace rendering {
namespace debug {

class Service {
public:
	void viewProjectionMatrix(const mat44f& matrix);

	void line(vec3f a,vec3f b,const vec4f& colour,bool depthtest = true);
	void line(const mat44f& matrix,vec3f a,vec3f b,const vec4f& colour,bool depthtest = true);
	void lines(const mat44f& matrix,vec3f* points,uint32 count,const vec4f& colour,bool depthtest = true);
	void axis(const mat44f& matrix,bool depthtest = true);//3D axis	
	void wireBox (const mat44f& matrix,vec3f min,vec3f max,const vec4f& colour,bool depthtest = true);
	void box(const mat44f& matrix,vec3f min,vec3f max,const vec4f& colour);
	void skeleton(const data::Bone* bones,size_t count,const vec4f& colour = vec4f(0.f,0.f,1.f,1.f),bool depthtest = false);

	void line     (vec2f a,vec2f b,const vec4f& colour);
	void wireRectangle(vec2f min,vec2f max,const vec4f& colour);
	void rectangle (vec2f min,vec2f max,const vec4f& colour);

	void render(Pipeline pipeline);
	void servicePreStep();
	void servicePostStep();
	Service(core::Allocator* allocator);
	~Service();

private:
	struct GeometryDestination {
		uint32 indexOffset;
		float* vertices;
		uint16* indices;
	};

	void mergeLines(core::BufferAllocator* threadLineVertices,core::BufferAllocator* threadLineIndices);
	void mergeTriangles(core::BufferAllocator* threadTriangleVertices,core::BufferAllocator* threadTriangleIndices);
	void getThreadLineBuffers(core::BufferAllocator*& vertices,core::BufferAllocator*& indices);
	void getThreadTriangleBuffers(core::BufferAllocator*& vertices,core::BufferAllocator*& indices);
	GeometryDestination allocateLines(uint32 vertexCount,uint32 indexCount,bool depthtest);
	GeometryDestination allocateTriangles(uint32 vertexCount,uint32 indexCount);

	mat44f viewProjection_;
	core::BufferAllocator threadBuffers_;
	core::BufferAllocator lines_;
	core::BufferAllocator lineIndices_;
	core::BufferAllocator triangles_;
	core::BufferAllocator triangleIndices_;
	core::Mutex* lineMutex_,*triangleMutex_,*threadBufferMutex_;
	//Adjust buffer sizes dynamically each frame
	size_t linesMaxSize, lineIndicesMaxSize, trianglesMaxSize,triangleIndicesMaxSize;
	Buffer   vbo_,ibo_;
	Mesh     mesh_;
	bool trianglesHaveOpacity_;
	
};

} }