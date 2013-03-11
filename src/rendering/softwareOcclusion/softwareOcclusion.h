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

	uint32 rasterizeAABB(vec3f min,vec3f max);

	void rasterizeTiles();

	//Tests if the AABB is occluded or not
	bool testAABB(vec3f min,vec3f max);

	void* getTexels(texture::Descriptor2D& descriptor);


	inline vec2i size() const;
private:
	void drawTriangle(vec2i v0,vec2i v1,vec2i v2,vec2i tileClock);
	void drawTriangle(BinnedTriangle& tri,vec2i tileClock);
	void binQuad(ScreenSpaceQuad& quad);
	void binTriangle(ScreenSpaceVertex& v0,ScreenSpaceVertex& v1,ScreenSpaceVertex& v2);
	void rasterizeTile(int32 x,int32 y,uint32 pass = 0);

	inline void getRay(vec3f& o,vec3f& dir,int32 x,int32 y);
	float* data_;
	mat44f viewProjection_;
	vec4f  clipSpaceToScreenSpaceMultiplier;
	float znear_,zfar_;
	vec3f cameraPos_,cameraDir_;float fov_;uint32 aabbFrontFaceMask;
	vec4f cameraRight_,cameraUp_;
	vec2i  center_;
	vec2i  size_;
	vec2i  tileSize_;vec2i tileCount_;
	uint32* tileTriangleCount_;
	ScreenSpaceQuad* tileQuads_;

	BinnedTriangle* triangleBins_;
	float*  texels_;
};
inline vec2i DepthBuffer::size() const { return size_; }

} }