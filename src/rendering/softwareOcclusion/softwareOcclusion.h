#pragma once

#include "../../core/math.h"
#include "../types.h"

namespace rendering {
namespace softwareOcclusion {

struct ScreenSpaceVertex;
struct ScreenSpaceQuad;
struct BinnedTriangle;

class DepthBuffer {
public:
	DepthBuffer(vec2i size = vec2i(0,0));
	~DepthBuffer();
	void clear(float far = 100000.0f);

	void setCamera(const mat44f& viewProjection,float znear,float zfar,float fov,vec3f cameraPos,vec3f cameraDir,vec3f cameraUp);

	void binAABB(vec3f min,vec3f max);

	uint32 rasterizeTiles();

	//Tests if the AABB is occluded or not
	bool testAABB(vec3f min,vec3f max);

	void* getTexels(texture::Descriptor2D& descriptor);


	inline vec2i size() const;
private:
	void drawTriangle(vec2i v0,vec2i v1,vec2i v2,vec2i tileClock);
	void drawTriangle(BinnedTriangle& tri,vec2i tileClock);
	void binQuad(ScreenSpaceQuad& quad);
	void binTriangle(const vec4f& v0,const vec4f& v1,const vec4f& v2);
	void binTriangles4Simd(vec4f vertices[12],uint32 count);
	void rasterizeTile(int32 x,int32 y,uint32 pass = 0);
	void rasterizeTile2x2(int32 x,int32 y,uint32 pass = 0);

	inline void getRay(vec3f& o,vec3f& dir,int32 x,int32 y);
	uint32 mode_;uint32 aabbFrontFaceMask;
	float* data_;	
	vec2i  center_;
	vec2i  size_;
	vec2i  tileSize_;vec2i tileCount_;
	uint32* tileTriangleCount_;
	BinnedTriangle* triangleBins_;
	uint32 triangleBufferOffset;
	vec4f triangleBufferStorage[4*3];
	float*  texels_;

	mat44f viewProjection_;
	vec4f  clipSpaceToScreenSpaceMultiplier;
	float znear_,zfar_;
	vec3f cameraPos_,cameraDir_;float fov_;
	vec4f cameraRight_,cameraUp_;
};
inline vec2i DepthBuffer::size() const { return size_; }

} }