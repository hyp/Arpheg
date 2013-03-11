
#include "../../core/memory.h"
#include "../../core/assert.h"
#include "softwareOcclusion.h"

namespace rendering {
namespace softwareOcclusion {

struct ScreenSpaceVertex {
	vec2i pos;
	float z;
	float w;
};
struct ScreenSpaceQuad {
	ScreenSpaceVertex v[4];
};

//A rasterizable triangle.
struct XY {
	//struct {
		int32 x, y;
	//};
	//uint32 xy;
};
struct BinnedTriangle {
	XY v[3];
	float z[3];
};

enum {
	kMaxQuadPerTile = 256,
	kMaxTrianglesPerTile = 512,
};

DepthBuffer::DepthBuffer(vec2i size){
	auto allocator= core::memory::globalAllocator();
	size_ = size;
	tileCount_ = vec2i(4,4);
	if(size.y%tileCount_.y){
		tileCount_.y = 5;
		if(size.y%tileCount_.y) tileCount_.y = 6;
	} 
	if(size.x%tileCount_.x){
		tileCount_.x = 5;
		if(size.x%tileCount_.x) tileCount_.x = 6;
	}
	tileSize_ = vec2i(size.x/tileCount_.x,size.y/tileCount_.y);
	assertRelease((size.x%tileCount_.x == 0) && (size.y%tileCount_.y == 0));

	center_ = vec2i(size.x/2,size.y/2);
	data_ = (float*)allocator->allocate(size_.x*size_.y*sizeof(float),alignof(vec4f));
	texels_ = nullptr;
	tileTriangleCount_ = (uint32*)allocator->allocate(tileCount_.x*tileCount_.y*sizeof(uint32),alignof(vec4f));
	tileQuads_ = (ScreenSpaceQuad*)allocator->allocate(tileCount_.x*tileCount_.y*kMaxQuadPerTile*sizeof(ScreenSpaceQuad),alignof(vec4f));
	triangleBins_ = (BinnedTriangle*)allocator->allocate(tileCount_.x*tileCount_.y*kMaxTrianglesPerTile*sizeof(BinnedTriangle),alignof(vec4f));

	// Half because the range is [-1,1] -> [0,2]
	clipSpaceToScreenSpaceMultiplier = vec4f(float(center_.x),float(center_.y),1.0f,1.0f);
}
DepthBuffer::~DepthBuffer() {
	core::memory::globalAllocator()->deallocate(data_);
}

#ifdef ARPHEG_ARCH_X86
static void clearDepthSSE(float* begin,size_t length,float value){
	enum { SimdLaneWidth = 4 };

	assertRelease(uintptr_t(begin)%16 == 0);
	auto rem = length % SimdLaneWidth;
	length = (length/SimdLaneWidth)*SimdLaneWidth;
	
	__m128 k = _mm_set_ps1(value);
	for(size_t i = 0;i<length;i+=SimdLaneWidth)
		_mm_store_ps(begin+i,k);
	if(rem == 0) return;
	for(size_t i = 0;i<rem;i++){
		begin[length+i] = value;
	}
}
#endif
static void clearDepth(float* begin,size_t length,float value){
#ifdef ARPHEG_ARCH_X86
	clearDepthSSE(begin,length,value);
#endif
}



void DepthBuffer::clear(float far) {
	clearDepth(data_,size_.x*size_.y,far);
	//Reset tiles.
	for(uint32 i = 0;i<tileCount_.x*tileCount_.y;++i) tileTriangleCount_[i] = 0;
	assertRelease(sizeof(ScreenSpaceVertex) == sizeof(vec4f));
}

const float maxFarDistance = 1000000.0f;
//Test AABBs for occlusion using ray tracing
float rayAABBDistance(vec3f min,vec3f max,vec3f rayOrig,vec3f rayDir){
	// r.dir is unit direction vector of ray
	vec3f dirfrac;
	dirfrac.x = 1.0f / rayDir.x;
	dirfrac.y = 1.0f / rayDir.y;
	dirfrac.z = 1.0f / rayDir.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (min.x - rayOrig.x)*dirfrac.x;
	float t2 = (max.x - rayOrig.x)*dirfrac.x;
	float t3 = (min.y - rayOrig.y)*dirfrac.y;
	float t4 = (max.y - rayOrig.y)*dirfrac.y;
	float t5 = (min.z - rayOrig.z)*dirfrac.z;
	float t6 = (max.z - rayOrig.z)*dirfrac.z;


	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
	if (tmax < 0){
		return maxFarDistance;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax){
		return maxFarDistance;
	}

	return  tmin;
}
bool rayAABBIntersect(vec3f min,vec3f max,vec3f rayOrig,vec3f rayDir,float& dist){
	// r.dir is unit direction vector of ray
	vec3f dirfrac;
	dirfrac.x = 1.0f / rayDir.x;
	dirfrac.y = 1.0f / rayDir.y;
	dirfrac.z = 1.0f / rayDir.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (min.x - rayOrig.x)*dirfrac.x;
	float t2 = (max.x - rayOrig.x)*dirfrac.x;
	float t3 = (min.y - rayOrig.y)*dirfrac.y;
	float t4 = (max.y - rayOrig.y)*dirfrac.y;
	float t5 = (min.z - rayOrig.z)*dirfrac.z;
	float t6 = (max.z - rayOrig.z)*dirfrac.z;


	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
	if (tmax < 0){
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax){
		return false;
	}

	dist = tmin;
	return true;
}

inline int32 orient2d(const vec2i a, const vec2i b, const vec2i c){
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}
void DepthBuffer::drawTriangle(vec2i v0,vec2i v1,vec2i v2,vec2i tilePos) {
	int32 minx = std::min(v0.x,std::min(v1.x,v2.x));
	int32 miny = std::min(v0.y,std::min(v1.y,v2.y));
	int32 maxx = std::max(v0.x,std::max(v1.x,v2.x));
	int32 maxy = std::max(v0.y,std::max(v1.y,v2.y));

	minx = std::max(minx,tilePos.x); miny = std::max(miny,tilePos.y);
	maxx = std::min(maxx,tilePos.x+tileSize_.x-1); maxy = std::min(maxy,tilePos.y+tileSize_.y-1);

	int32 a01 = v0.y - v1.y; int32 b01 = v1.x - v0.x;
	int32 a12 = v1.y - v2.y; int32 b12 = v2.x - v1.x;
	int32 a20 = v2.y - v0.y; int32 b20 = v0.x - v2.x;

	vec2i p(minx,miny);

	auto w0_row = orient2d(v1,v2,p);
	auto w1_row = orient2d(v2,v0,p);
	auto w2_row = orient2d(v0,v1,p);

	for(p.y = miny;p.y<=maxy;++p.y){
		auto w0 = w0_row;
		auto w1 = w1_row;
		auto w2 = w2_row;

		for(p.x = minx;p.x<=maxx;++p.x){
			if((w0|w1|w2) >= 0){
				data_[p.x+p.y*size_.x] = 0.5f;
			}

			w0+=a12;
			w1+=a20;
			w2+=a01;
		}
		w0_row += b12;
		w1_row += b20;
		w2_row += b01;
	}
}
void* DepthBuffer::getTexels(texture::Descriptor2D& descriptor) {
	descriptor.width = size_.x;
	descriptor.height = size_.y;
	descriptor.format = texture::FLOAT_R_32;
	if(!texels_){
		texels_ = (float*)core::memory::globalAllocator()->allocate(size_.x*size_.y*sizeof(float),alignof(vec4f));
	}

	//Copy row by row
	size_t pixelsPerTile = tileSize_.x*tileSize_.y;
	float* dest = texels_;
	for(int32 y = 0;y<size_.y;++y){
		int32 tileY = y/tileSize_.y;
		int32 localYOffset = (y - tileY*tileSize_.y)*tileSize_.x;
		
		for(int32 tileX = 0;tileX < tileCount_.x;++tileX){
			float* tilePixels    = data_ + tileX*pixelsPerTile + tileY*pixelsPerTile*tileCount_.x;
			float* tileRowPixels = tilePixels + localYOffset;
			memcpy(dest,tileRowPixels,tileSize_.x*sizeof(float));
			dest+=tileSize_.x;
		}
	}

	
	/*for(int32 y = 0;y<tileCount_.y;++y){
	for(int32 x = 0;x<tileCount_.x;++x){
		float* tilePixels= data_ + x*pixelsPerTile + y*pixelsPerTile;
		//copy rows
		for(int32 yy = 0;yy < tileSize_.y;++yy){
			float* rowPixels = tilePixels + yy*tileSize_.x;
			float* dest = texels_ + x*tileSize_.x + (y*tileSize_.y+yy)*size_.x;
			memcpy(dest,rowPixels,tileSize_.x*sizeof(float));
		}
	} }*/
	//float* tilePixels = data_ + tilePos.x*tileSize_.y + tilePos.y*tileSize_.x;

	//memcpy(texels_,data_,size_.x*size_.y*sizeof(float));
	return texels_;
}

#define RIGHT	0
#define LEFT	1
#define MIDDLE	2
#define NUMDIM 3

/**********************************************************************/
/* Ray-Box Intersection by Andrew Woo from "Graphics Gems", Academic  */
/* Press, 1990                                                        */
/* Returns true if intersects, and intersect distance.                */
/**********************************************************************/
bool RayBoundingBox(vec3f min,vec3f max,vec3f rayo,vec3f rayd,float& dist)
{
  char quadrant[4];
  int i = 0;

  float maxT[3];
  float candidatePlane[3];
  /* Find candidate planes; this loop can be avoided if
     rays cast all from the eye (assume perpsective view) */
  float* ray_orig = &rayo.x;
  float* ray_dir = &rayd.x;
  float* minB = &min.x;
  float* maxB = &max.x;

  for (i=0; i<3; i++){
    if (ray_orig[i] < minB[i]) {
      quadrant[i] = LEFT;
      candidatePlane[i] = minB[i];
    } else if (ray_orig[i] > maxB[i]) {
      quadrant[i] = RIGHT;
      candidatePlane[i] = maxB[i];
    } else	{
      quadrant[i] = MIDDLE;
	}
  }
  
  /* Calculate the distances to candidate planes */
  for (i=0; i<NUMDIM; i++)
    if (quadrant[i] != MIDDLE && ray_dir[i] !=0.0)
      maxT[i] = (candidatePlane[i]-ray_orig[i]) / ray_dir[i];
    else
      maxT[i] = -1.;
  
  /* Get largest of the maxT's for final choice of intersection */
  int whichPlane = 0;
  for (i = 1; i < NUMDIM; i++){
    if (maxT[whichPlane] < maxT[i])
      whichPlane = i;
  }
  
  /* Check if final candidate actually inside box */
  if (maxT[whichPlane] < 0.0) 
    return false;
  for (i = 0; i < NUMDIM; i++){
    if (whichPlane != i) {
      auto coord = ray_orig[i] + maxT[whichPlane] * ray_dir[i];
      if ((quadrant[i] == RIGHT && coord < minB[i]) ||
	  (quadrant[i] == LEFT && coord > maxB[i]))
		return false;
    } 
  }

  dist = maxT[whichPlane];
  return true;
}

//Calculates the mask to determine which faces on aabb are visible(front facing).
// Box sides:
// 0,1 XY
// 2,3 XZ
// 4,5 YZ
static uint32 calculateBoxVisibleFaceMask(vec3f cameraDir){
	uint32 mask = 0;
	//XY
	if(cameraDir.z >= 0.0f){
		mask |= 1;
	} else mask |= 2;
	//XZ
	if(cameraDir.y >= 0.0f){
		mask |= 8;
	} else mask |= 8;
	//YZ
	if(cameraDir.x >= 0.0f){
		mask |= 0x10;
	} else mask |= 0x20;
	return mask;
}
inline bool   isBoxFaceHidden(uint32 mask,uint32 i){
	return (mask&(1<<i)) == 0;
}
static inline uint32 makeBoxIndex(uint32 a,uint32 b,uint32 c,uint32 d){
	return a|(b<<8)|(c<<16)|(d<<24);
}

void DepthBuffer::setCamera(const mat44f& viewProjection,float znear,float zfar,float fov,vec3f cameraPos,vec3f cameraDir,vec3f cameraUp){
	viewProjection_ = viewProjection;
	znear_ = znear;
	zfar_ = zfar;
	fov_ = fov;
	float aspect = float(size_.x)/float(size_.y);
	cameraPos_ = cameraPos;
	cameraDir_ = cameraDir;

	vec3f cameraRight = cameraDir.cross(cameraUp).normalize();
	cameraUp = cameraRight.cross(cameraDir);
	//
	auto upLength = tanf(fov/2.0f)*znear;
	auto rightLength = upLength * (aspect);
	cameraRight = cameraRight*rightLength;
	cameraUp = cameraUp*upLength;

	cameraRight_ = vec4f(cameraRight.x,cameraRight.y,cameraRight.z,1);
	cameraUp_  = vec4f(cameraUp.x,cameraUp.y,cameraUp.z,1);

	aabbFrontFaceMask = calculateBoxVisibleFaceMask(cameraDir);
}
void DepthBuffer::getRay(vec3f& o,vec3f& dir,int32 x,int32 y) {
	x-=center_.x;
	y-=center_.y;
	float cx = ( float(x) / float(center_.x) );
	float cy = ( float(y) / float(center_.y) );
	o = cameraPos_ + cameraDir_*znear_ + (cameraRight_*cx).xyz() + (cameraUp_*cy).xyz();
	dir = o - cameraPos_;
}

// Box setup 
static inline void gatherBoxVertices(vec4f vertices[8],vec3f min,vec3f max){
#ifdef ARPHEG_ARCH_X86
	auto vvertices = (float*)vertices;
	__m128 vmin = _mm_setr_ps(min.x,min.y,min.z,1.0f);
	__m128 vmax = _mm_setr_ps(max.x,max.y,max.z,1.0f);
	__m128 r;
	_mm_store_ps(vvertices,vmin); //min
	r = _mm_move_ss(vmin,vmax); 
	_mm_store_ps(vvertices+4,r);//max.x,min.y,min.z
	r = _mm_shuffle_ps(vmax,vmin,_MM_SHUFFLE(3,2,1,0)); 
	_mm_store_ps(vvertices+8,r);//max.x,max.y,min.z,1
	r = _mm_move_ss(r,vmin); 
	_mm_store_ps(vvertices+12,r);//min.x,max.y,min.z

	r = _mm_shuffle_ps(vmin,vmax,_MM_SHUFFLE(3,2,1,0)); 
	_mm_store_ps(vvertices+16,r);//min.x,min.y,max.z
	r = _mm_move_ss(r,vmax);
	_mm_store_ps(vvertices+20,r); //max.x,min.y,max.z
	_mm_store_ps(vvertices+24,vmax);//max
	r = _mm_move_ss(vmax,vmin); 
	_mm_store_ps(vvertices+28,r); //min.x,max.y,max.z
#else
	vertices[0] = vec4f(min.x,min.y,min.z,1);
	vertices[1] = vec4f(max.x,min.y,min.z,1);
	vertices[2] = vec4f(max.x,max.y,min.z,1);
	vertices[3] = vec4f(min.x,max.y,min.z,1);
	vertices[4] = vec4f(min.x,min.y,max.z,1);
	vertices[5] = vec4f(max.x,min.y,max.z,1);
	vertices[6] = vec4f(max.x,max.y,max.z,1);
	vertices[7] = vec4f(min.x,max.y,max.z,1);
#endif
}
static inline void transformBoxVertices(vec4f vertices[8],const mat44f& matrix){
	//Transform the vertices to homogenous coordinates
	for(uint32 i =0;i<8;i++){
		vertices[i] = matrix*vertices[i];
	}
}
static inline void boxVerticesToScreenVertices(vec4f vertices[8],const vec4f& screenCenterMul,vec2i screenCenter){
#ifdef ARPHEG_ARCH_X86
	__m128 screenSpaceMul = _mm_load_ps((float*)&screenCenterMul.x);
	__m128i screenCenterOffset = _mm_setr_epi32(screenCenter.x,screenCenter.y,0,0);
	//__m128 nearClip = _mm_set_ps1(-1.0f);
	for(uint32 i = 0;i<8;++i){
		__m128 hv = _mm_load_ps((float*)(vertices + i));
		
		__m128 w  = _mm_shuffle_ps(hv,hv,_MM_SHUFFLE(3,3,3,3)); //get the w component
		hv = _mm_div_ps(hv,w); //Project XYZW to clip space (divide by w)
		//__m128 z  = _mm_shuffle_ps(hv,hv,_MM_SHUFFLE(2,2,2,2));
		hv = _mm_mul_ps(hv,screenSpaceMul); //XY to screen space
		//__m128 mNoNearClip = _mm_cmpge_ps(z, nearClip );
		w  = _mm_shuffle_ps(w,hv,_MM_SHUFFLE(2,2,0,0));//w,w,z,z
		//Truncate conversion to screen space
		__m128i screenSpace = _mm_cvttps_epi32(hv);
		screenSpace = _mm_add_epi32(screenSpace,screenCenterOffset);//XY to screen space
		hv = _mm_castsi128_ps(screenSpace);
		hv = _mm_shuffle_ps(hv,w,_MM_SHUFFLE(0,2,1,0));//x,y,z,w

		//Set to all-0 if near-clipped
		//hv = _mm_and_ps(hv, mNoNearClip);

		_mm_store_ps((float*)(vertices + i),hv);
	}
#else
	ScreenSpaceVertex* screenVerts= (ScreenSpaceVertex*)vertices;
	for(uint32 i =0;i<8;++i){
		vertices[i] = vertices[i] * (1.0f/vertices[i].w) ;
		auto v = vertices[i] * screenCenterMul;
		screenVerts[i].pos = vec2i(int32(v.x),int32(v.y))+screenCenter;
	}
#endif
}
static inline uint32 extractBoxQuads(ScreenSpaceQuad faces[6],ScreenSpaceVertex vertices[8],uint32 aabbFrontFaceMask){
	static uint32 indices[6] = {
		makeBoxIndex(0,1,2,3),makeBoxIndex(4,5,6,7), //XY
		makeBoxIndex(1,0,4,5),makeBoxIndex(2,3,7,6), //XZ
		makeBoxIndex(0,3,7,4),makeBoxIndex(1,2,6,5)  //YZ
	};
	uint32 faceCount = 0;
	for(uint32 i = 0;i<6;++i){
		//TODO: 
		if(isBoxFaceHidden(aabbFrontFaceMask,i)) continue;
		auto index = indices[i];
		faces[faceCount].v[0] = vertices[index&0xFF];
		faces[faceCount].v[1] = vertices[(index>>8)&0xFF];
		faces[faceCount].v[2] = vertices[(index>>16)&0xFF];
		faces[faceCount].v[3] = vertices[(index>>24)];
		faceCount++;
	}
	return faceCount;
}


void DepthBuffer::binTriangle(ScreenSpaceVertex& v0,ScreenSpaceVertex& v1,ScreenSpaceVertex& v2) {
	vec2i min,max;
	min.x = std::max(0,std::min(std::min(v0.pos.x,v1.pos.x),v2.pos.x));
	min.y = std::max(0,std::min(std::min(v0.pos.y,v1.pos.y),v2.pos.y));
	max.x = std::min(size_.x-1,std::max(std::max(v0.pos.x,v1.pos.x),v2.pos.x));
	max.y = std::min(size_.y-1,std::max(std::max(v0.pos.y,v1.pos.y),v2.pos.y));
	//Convert to tile coords
	min.x = min.x/tileSize_.x;//std::min(0,min.x / tileSize_.x);
	min.y = min.y/tileSize_.y;//std::min(0,min.y / tileSize_.y);
	max.x = max.x/tileSize_.x;//std::max(tileCount_.x-1,max.x / tileSize_.x);
	max.y = max.y/tileSize_.y;//std::max(tileCount_.y-1,max.y / tileSize_.y);
	

	auto area = (v1.pos.x - v0.pos.x) * (v2.pos.y - v0.pos.y) - (v0.pos.x - v2.pos.x) * (v0.pos.y - v1.pos.y);
	if(area <= 0.0f) return;//skip if the area is zero
	/*if(v0.w == 0.0f || v1.w == 0.0f || v2.w == 0.0f){
		area = 1.0f;
		return;
	}*/
	float oneOverArea = 1.0f/float(area);

	for(int32 y = min.y;y<=max.y;++y){
	for(int32 x = min.x;x<=max.x;++x){
		auto tileIndex = x + y*tileCount_.x;
		auto count = tileTriangleCount_[tileIndex];
		if(count >= kMaxTrianglesPerTile) continue;
		tileTriangleCount_[tileIndex]++;
		BinnedTriangle& triangle =*( triangleBins_ + count + x*kMaxTrianglesPerTile + y*tileCount_.x*kMaxTrianglesPerTile);

		triangle.v[0].x = v0.pos.x;
		triangle.v[0].y = v0.pos.y;
		triangle.v[1].x = v1.pos.x;
		triangle.v[1].y = v1.pos.y;
		triangle.v[2].x = v2.pos.x;
		triangle.v[2].y = v2.pos.y;
		triangle.z[0] = v0.z;
		triangle.z[1] = (v1.z-v0.z)*oneOverArea;
		triangle.z[2] = (v2.z-v0.z)*oneOverArea;
	} }
}
void DepthBuffer::binQuad(ScreenSpaceQuad& quad) {
	//Find the quad bounds
	binTriangle(quad.v[0],quad.v[1],quad.v[2]);
	binTriangle(quad.v[2],quad.v[3],quad.v[0]);
	return;
	vec2i min = quad.v[0].pos;vec2i max = quad.v[0].pos;
	for(uint32 i = 1;i<4;++i){
		auto v = quad.v[i].pos;
		min.x = std::min(min.x,v.x);
		min.y = std::min(min.y,v.y);
		max.x = std::max(max.x,v.x);
		max.y = std::max(max.y,v.y);
	}
	min.x = std::min(0,min.x / tileSize_.x);
	min.y = std::min(0,min.y / tileSize_.y);
	max.x = std::max(tileCount_.x-1,max.x / tileSize_.x);
	max.y = std::max(tileCount_.y-1,max.y / tileSize_.y);

	//Area of a triangle(same for two).
	auto area = (quad.v[1].pos.x - quad.v[0].pos.x) * (quad.v[2].pos.y - quad.v[0].pos.y) - (quad.v[0].pos.x - quad.v[2].pos.x) * (quad.v[0].pos.y - quad.v[1].pos.y);
	float oneOverArea1 = 1.0f/float(area);
	area = (quad.v[3].pos.x - quad.v[2].pos.x) * (quad.v[0].pos.y - quad.v[2].pos.y) - (quad.v[2].pos.x - quad.v[0].pos.x) * (quad.v[2].pos.y - quad.v[3].pos.y);
	float oneOverArea2 = oneOverArea1;// 1.0f/fabs(float(area));

	for(uint32 y = min.y;y<=max.y;++y){
	for(uint32 x = min.x;x<=max.x;++x){
		auto tileIndex = x + y*tileCount_.x;
		auto count = tileTriangleCount_[tileIndex];
		if((count+2) > kMaxTrianglesPerTile) continue;
		tileTriangleCount_[tileIndex]+=2;
		BinnedTriangle triangle;
		//first triangle
		triangle.v[0].x = quad.v[0].pos.x;
		triangle.v[0].y = quad.v[0].pos.y;
		triangle.v[1].x = quad.v[1].pos.x;
		triangle.v[1].y = quad.v[1].pos.y;
		triangle.v[2].x = quad.v[2].pos.x;
		triangle.v[2].y = quad.v[2].pos.y;
		triangle.z[0] = quad.v[0].z;
		triangle.z[1] = (quad.v[1].z-triangle.z[0])*oneOverArea1;
		triangle.z[2] = (quad.v[2].z-triangle.z[0])*oneOverArea1;
		

		auto triIndex = count + x*kMaxTrianglesPerTile + y*tileCount_.x*kMaxTrianglesPerTile;
		triangleBins_[triIndex] = triangle;
		//Second triangle
		triangle.v[0] = triangle.v[2];
		triangle.v[1].x = quad.v[3].pos.x;
		triangle.v[1].y = quad.v[3].pos.y;
		triangle.v[2].x = quad.v[0].pos.x;
		triangle.v[2].y = quad.v[0].pos.y;
		triangle.z[0] = quad.v[2].z;
		triangle.z[1] = (quad.v[3].z-triangle.z[0])*oneOverArea2;
		triangle.z[2] = (quad.v[0].z-triangle.z[0])*oneOverArea2;

		/*auto v0 = triangle.v[0];
		triangle.v[0] = triangle.v[2];
		triangle.v[1].x = quad.v[3].pos.x;
		triangle.v[1].x = quad.v[3].pos.y;
		auto z3 = quad.v[3].z;
		triangle.v[2] = v0;
		auto z0 = triangle.z[0];
		triangle.z[0] = triangle.z[2];
		triangle.z[1] = z3;
		triangle.z[2] = z0;*/
		
		triangleBins_[triIndex+1] = triangle;
	} }
}

void DepthBuffer::rasterizeTiles() {
	for(int32 y = 0;y<tileCount_.y;++y){
	for(int32 x = 0;x<tileCount_.x;++x){
		rasterizeTile(x,y,0);
	} }
}
void DepthBuffer::drawTriangle(BinnedTriangle& tri,vec2i tilePos) {
	vec2i v0(tri.v[0].x,tri.v[0].y);
	vec2i v1(tri.v[1].x,tri.v[1].y);
	vec2i v2(tri.v[2].x,tri.v[2].y);
	float zz[3] = { tri.z[0],tri.z[1],tri.z[2] };

	int32 minx = std::min(v0.x,std::min(v1.x,v2.x));
	int32 miny = std::min(v0.y,std::min(v1.y,v2.y));
	int32 maxx = std::max(v0.x,std::max(v1.x,v2.x));
	int32 maxy = std::max(v0.y,std::max(v1.y,v2.y));

	minx = std::max(minx,tilePos.x); miny = std::max(miny,tilePos.y);
	maxx = std::min(maxx,tilePos.x+tileSize_.x-1); maxy = std::min(maxy,tilePos.y+tileSize_.y-1);

	int32 a01 = v0.y - v1.y; int32 b01 = v1.x - v0.x;
	int32 a12 = v1.y - v2.y; int32 b12 = v2.x - v1.x;
	int32 a20 = v2.y - v0.y; int32 b20 = v0.x - v2.x;

	vec2i p(minx,miny);

	auto w0_row = orient2d(v1,v2,p);
	auto w1_row = orient2d(v2,v0,p);
	auto w2_row = orient2d(v0,v1,p);

	float* tilePixels = data_ + tilePos.x*tileSize_.y + (tilePos.y*tileSize_.x)*tileCount_.x;
	
	int32 idx2  = minx-tilePos.x + (miny - tilePos.y)*tileSize_.x;
	int32 spanx = maxx-minx;
	
	for(int32 endIdx2 = idx2+(tileSize_.x)*(maxy-miny);idx2<=endIdx2;idx2+=tileSize_.x){
		auto w0 = w0_row;
		auto w1 = w1_row;
		auto w2 = w2_row;

		//float betaf = float(w1);
		//float gamaf = float(w2);
		//float depth = zz[0] + betaf*zz[1] + gamaf*zz[2];
		//float depthInc = float(a20)*zz[1] + float(a01)*zz[2];
		auto idx = idx2;
		for(int32 endIdx = idx+spanx;idx<=endIdx;++idx){
			if((w0|w1|w2) >= 0){
				float betaf = float(w1);
				float gamaf = float(w2);
				float depth = zz[0] + betaf*zz[1] + gamaf*zz[2];

				auto d = tilePixels[idx];
				d = depth<d?depth:d;
				tilePixels[idx] = d;
			}

			w0+=a12;
			w1+=a20;
			w2+=a01;
			//depth+=depthInc;
		}
		w0_row += b12;
		w1_row += b20;
		w2_row += b01;
	}
}
void DepthBuffer::rasterizeTile(int32 x,int32 y,uint32 pass) {
	enum { SimdLaneWidth = 1 };

	if(pass == 0){
		//init tile(clear depth).	
		//auto tilePixels = data_ + x*tileSize_.x*tileSize_.y + (y*tileSize_.x*tileSize_.y)*tileCount_.x;
		//clearDepth(tilePixels,tileSize_.x*tileSize_.y,1.0f);
	}
	auto tileIndex = x + y*tileCount_.x;
	auto count = tileTriangleCount_[tileIndex];
	tileTriangleCount_[tileIndex] = 0;
	auto faces = triangleBins_ + x*kMaxTrianglesPerTile + y*tileCount_.x*kMaxTrianglesPerTile;
	vec2i tilePos(x*tileSize_.x,y*tileSize_.y);
	
	for(uint32 i = 0;i<count;i += SimdLaneWidth){
		drawTriangle(faces[i],tilePos);
	}
}

uint32 DepthBuffer::rasterizeAABB(vec3f min,vec3f max) {
	//Transform the AABB vertices to Homogenous clip space
	vec4f vertices[8];
	gatherBoxVertices(vertices,min,max);
	transformBoxVertices(vertices,viewProjection_);
	boxVerticesToScreenVertices(vertices,clipSpaceToScreenSpaceMultiplier,center_);
	ScreenSpaceQuad faces[6];
	auto faceCount = extractBoxQuads(faces,(ScreenSpaceVertex*)vertices,aabbFrontFaceMask);
	for(uint32 i = 0;i<faceCount;++i)
		binQuad(faces[i]);
	return faceCount;
}

bool DepthBuffer::testAABB(vec3f min,vec3f max){
	//Transform the AABB vertices to Homogenous clip space
	vec4f vertices[8];
	vertices[0] = vec4f(min.x,min.y,min.z,1);
	vertices[1] = vec4f(max.x,min.y,min.z,1);
	vertices[2] = vec4f(max.x,max.y,min.z,1);
	vertices[3] = vec4f(min.x,max.y,min.z,1);
	vertices[4] = vec4f(min.x,min.y,max.z,1);
	vertices[5] = vec4f(max.x,min.y,max.z,1);
	vertices[6] = vec4f(max.x,max.y,max.z,1);
	vertices[7] = vec4f(min.x,max.y,max.z,1);
	vec4f clipMin,clipMax;
	for(uint32 i =0;i<8;++i){
		vertices[i] = viewProjection_ * vertices[i]; 
		vertices[i] = vertices[i] * (1.0f/vertices[i].w);
		//Homogenous coordinates => non-homogenous coordinates
		//Determine the min/max for the screen aabb
		clipMin = vec4f::min(vertices[i],clipMin);
		clipMax = vec4f::max(vertices[i],clipMax);
	}
	//Determine the screen aabb which covers the box
	//Clip space coordinates => screen coordinates [-1,1] -> [-320,320] -> [0,640]
	clipMin = vec4f::max(clipMin,vec4f(-1.0f,-1.0f,-1.0f,-1.0f))*clipSpaceToScreenSpaceMultiplier;
	clipMax = vec4f::min(clipMax,vec4f(1.0f,1.0f,1.0f,1.0f))*clipSpaceToScreenSpaceMultiplier;
	vec2i screenMin = vec2i(int32(clipMin.x),int32(clipMin.y))+center_;
	vec2i screenMax = vec2i(int32(clipMax.x),int32(clipMax.y))+center_;
	
	//Iterate over the pixels
	for(;screenMin.y < screenMax.y;screenMin.y++){
	for(int32 x = screenMin.x;x<screenMax.x;x++){
		//Fetch the distance value for the current pixel.
		float depth = data_[x+screenMin.y*size_.x];
		//Compute the distance to the aabb (raytrace)
		vec3f rayo,rayd;
		getRay(rayo,rayd,x,screenMin.y);
		float dist;
		if(!rayAABBIntersect(min,max,rayo,rayd,dist)) continue;
		//Convert the distance from view space to depth space [-1,1]
		dist = ((dist-znear_)/zfar_ ) * 2.0f - 1.0f;
		//Compare the values.
		//if(dist <= depth){
			//return true;
			data_[x+screenMin.y*size_.x] = dist;
		//}
	} }
	return false;
}

} }