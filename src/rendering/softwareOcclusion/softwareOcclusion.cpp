
#include "../../core/memory.h"
#include "../../core/assert.h"
#include "softwareOcclusion.h"

namespace rendering {
namespace softwareOcclusion {

struct Tile {

};

DepthBuffer::DepthBuffer(vec2i size){
	auto allocator= core::memory::globalAllocator();
	size_ = size;
	center_ = vec2i(size.x/2,size.y/2);
	data_ = (float*)allocator->allocate(size_.x*size_.y*sizeof(float),alignof(vec4f));
	texels_ = nullptr;

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
void DepthBuffer::drawTriangle(vec2i v0,vec2i v1,vec2i v2) {
	int32 minx = std::min(v0.x,std::min(v1.x,v2.x));
	int32 miny = std::min(v0.y,std::min(v1.y,v2.y));
	int32 maxx = std::max(v0.x,std::max(v1.x,v2.x));
	int32 maxy = std::max(v0.y,std::max(v1.y,v2.y));

	minx = std::max(minx,0); miny = std::max(miny,0);
	maxx = std::min(maxx,size_.x-1); maxy = std::min(maxy,size_.y-1);

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
	memcpy(texels_,data_,size_.x*size_.y*sizeof(float));
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
		mask |= 4;
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

void DepthBuffer::setCamera(const mat44f& viewProjection,float znear,float zfar,float aspect,float fov,vec3f cameraPos,vec3f cameraDir,vec3f cameraUp){
	viewProjection_ = viewProjection;
	znear_ = znear;
	zfar_ = zfar;
	fov_ = fov;
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
uint32 DepthBuffer::rasterizeAABB(vec3f min,vec3f max) {
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

	vec4f clipMin(2,2,2,2);
	vec4f clipMax(-2.0f,-2.0f,-2.0f,-2.0f);

	struct SV {
		vec2i pos;
		float z;
		float dummy_;
	};
	SV screenVerts[8];

	for(uint32 i =0;i<8;++i){
		vertices[i] = viewProjection_ * vertices[i]; 
		vertices[i] = vertices[i] * (1.0f/vertices[i].w) ;
		auto v = vertices[i] * clipSpaceToScreenSpaceMultiplier;
		screenVerts[i].pos = vec2i(int32(v.x),int32(v.y))+center_;
		screenVerts[i].z = vertices[i].z;
		//Homogenous coordinates => non-homogenous coordinates
		//Determine the min/max for the screen aabb
		//clipMin = vec4f::min(vertices[i],clipMin);
		//clipMax = vec4f::max(vertices[i],clipMax);
	}


	//Extract AABB faces.
	struct Quad {
		vec2i v[4];
	};
	Quad faces[6];
	uint32 faceCount = 0;

	static uint32 indices[6] = {
		makeBoxIndex(0,1,2,3),makeBoxIndex(4,5,6,7), //XY
		makeBoxIndex(1,0,4,5),makeBoxIndex(3,2,6,7), //XZ
		makeBoxIndex(0,3,7,4),makeBoxIndex(1,2,6,5)  //YZ
	};

	for(uint32 i = 0;i<6;++i){
		if(isBoxFaceHidden(aabbFrontFaceMask,i)) continue;
		auto index = indices[i];
		faces[faceCount].v[0] = screenVerts[index&0xFF].pos;
		faces[faceCount].v[1] = screenVerts[(index>>8)&0xFF].pos;
		faces[faceCount].v[2] = screenVerts[(index>>16)&0xFF].pos;
		faces[faceCount].v[3] = screenVerts[(index>>24)].pos;
		faceCount++;
	}
	
	
	for(uint32 i = 0;i<faceCount;++i){
		drawTriangle(faces[i].v[0],faces[i].v[1],faces[i].v[2]);
		drawTriangle(faces[i].v[2],faces[i].v[3],faces[i].v[0]);
	}
	return faceCount;


	//Determine the screen aabb which covers the box
	//Clip space coordinates => screen coordinates [-1,1] -> [-320,320] -> [0,640]
	clipMin = vec4f::max(clipMin,vec4f(-1.0f,-1.0f,-1.0f,-1.0f))*clipSpaceToScreenSpaceMultiplier;
	clipMax = vec4f::min(clipMax,vec4f(1.0f,1.0f,1.0f,1.0f))*clipSpaceToScreenSpaceMultiplier;
	vec2i screenMin = vec2i(int32(clipMin.x),int32(clipMin.y))+center_;
	vec2i screenMax = vec2i(int32(clipMax.x),int32(clipMax.y))+center_;

	vec2i tri[8];
	for(int i =0;i<8;++i){
		auto s = vertices[i]*clipSpaceToScreenSpaceMultiplier;
		tri[i] = vec2i(int32(s.x),int32(s.y)) + center_;
	}


	//Iterate over the pixels
	uint32 coverage = (screenMax.x-screenMin.x)*(screenMax.y-screenMin.y);
	
	vec2f begin = vec2f(clipMin.x,clipMin.y);
	vec2f diff  = vec2f(1.0f/float(center_.x),1.0f/float(center_.y));
	for(;screenMin.y < screenMax.y;screenMin.y++,begin.y+=diff.y){
		float* dd = data_ + (screenMin.y)*size_.x;
		begin.x = clipMin.x;
	for(int32 x = screenMin.x;x<screenMax.x;x++,begin.x+=diff.x){
		//Compute the distance to the aabb (raytrace)

		//vec3f rayo = cameraPos_ + cameraDir_*znear_ + (cameraRight_*begin.x).xyz() + (cameraUp_*begin.y).xyz();
		//vec3f rayd = rayo - cameraPos_;

		vec3f rayo,rayd;
		getRay(rayo,rayd,x,screenMin.y);
		float dist;
		
		if(rayAABBIntersect(min,max,rayo,rayd,dist)){
			//Compare the values.
			//float depth =  dd[x];
			//if(dist < depth){
			//	dd[x] = dist;
			//}
			dd[x] = 0.5f;
		}
	} }
	return coverage;
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
		float dist = rayAABBDistance(min,max,rayo,rayd);
		//Convert the distance from view space to depth space [-1,1]
		//dist = ((dist-znear_)/zfar_ ) * 2.0f - 1.0f;
		//Compare the values.
		if(dist <= depth){
			return true;
		}
	} }
	return false;
}

} }