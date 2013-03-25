// Provides a thread safe service for debug rendering (wire boxes,lines, coordinate axises, etc)

#pragma once

#include "types.h"
#include "../core/math.h"
#include "../core/memory.h"
#include "../data/types.h"

namespace rendering {
namespace debug {

class Service {
public:
	void viewProjectionMatrix(const mat44f& matrix);

	void line(vec3f a,vec3f b,const vec4f& colour,bool depthtest = true);
	void line(vec3f a,vec3f b,uint32 colour,bool depthtest = true);
	void line(const mat44f& matrix,vec3f a,vec3f b,const vec4f& colour,bool depthtest = true);
	void lines(const mat44f& matrix,vec3f* points,uint32 count,const vec4f& colour,bool depthtest = true);
	void node(const mat44f& matrix,bool depthtest = true);//3D axis	
	void box (const mat44f& matrix,vec3f min,vec3f max,const vec4f& colour,bool depthtest = true);
	void sphere(const mat44f& matrix,vec3f position,float radius,const vec4f& colour,bool depthtest = true);
	void skeleton(const mat44f& matrix,const data::Mesh* mesh,const data::Transformation3D* nodes,const vec4f& colour = vec4f(0.f,0.f,1.f,1.f),bool depthtest = false);

	void line     (vec2f a,vec2f b,const vec4f& colour);
	void rectangle (vec2f min,vec2f max,const vec4f& colour);

	void pipeline(const data::Pipeline* pipeline);
	void pipeline(Pipeline pipeline);
	void render  (const mat44f& viewProjection);
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

	GeometryDestination allocateLines(uint32 vertexCount,uint32 indexCount,bool depthtest);

	mat44f viewProjection_;
	core::BufferAllocator lines_;
	core::BufferAllocator linesDepthTest_;

	//Adjust buffer sizes dynamically each frame
	size_t linesMaxSize, linesDepthTestMaxSize;
	Buffer   vbo_,ibo_;
	Mesh     mesh_;
	Pipeline pipeline_;
	Pipeline::Constant pipelineModelView;
};

} }