//Thanks to fgiesen.wordpress.com and Intel for the occlusion culling demo.

#include "../../core/memory.h"
#include "../../core/assert.h"
#include "../../core/bufferStringStream.h"
#include "../../services.h"
#include "../../application/logging.h"
#include "../../application/tasking.h"
#include "softwareOcclusion.h"

#ifdef ARPHEG_ARCH_X86
	#include "helperSSE.h"
#endif

namespace rendering {
namespace softwareOcclusion {

struct ScreenSpaceQuad {
	vec4f v[4];
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
	kMaxTrianglesPerTile = 1024*2,
	
	kModeDepthFlat = 0,   //Depth buffer pixels are stored in rows 
	kModeDepthPackedTiles,//Depth buffer pixels are stored in rows for each tile
	kModeDepthPackedQuads,//Depth buffer pixels are stored in 2x2 quads ((0,0),(1,0),(0,1),(1,1))
};

DepthBuffer::DepthBuffer(vec2i size){
	mode_ = kModeDepthPackedQuads;
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
	assertRelease(size.x%4 == 0);//Verify that each row will be aligned by 16
	assertRelease(size.x%2 == 0 && size.y%2 == 0);


	center_ = vec2i(size.x/2,size.y/2);
	data_ = (float*)allocator->allocate(size_.x*size_.y*sizeof(float),alignof(vec4f));
	texels_ = nullptr;
	tileTriangleCount_ = (uint32*)allocator->allocate(tileCount_.x*tileCount_.y*sizeof(uint32),alignof(vec4f));
	triangleBins_ = (BinnedTriangle*)allocator->allocate(tileCount_.x*tileCount_.y*kMaxTrianglesPerTile*sizeof(BinnedTriangle),alignof(vec4f));

	// Half because the range is [-1,1] -> [0,2]
	clipSpaceToScreenSpaceMultiplier = vec4f(float(center_.x),float(center_.y),1.0f,1.0f);

	using namespace core::bufferStringStream;
	Formatter fmt;
	printf(fmt.allocator,"Created depth buffer for software occlusion testing\n  Width: %d Height: %d\n  Tile width: %d Tile Height: %d Tile count: %d",size_.x,size_.y,tileSize_.x,tileSize_.y,tileCount_.x*tileCount_.y);
	services::logging()->information(asCString(fmt.allocator));
}
DepthBuffer::~DepthBuffer() {
	auto allocator_ = core::memory::globalAllocator();
	allocator_->deallocate(data_);
	allocator_->deallocate(triangleBins_);
	allocator_->deallocate(tileTriangleCount_);
	if(texels_){
		allocator_->deallocate(texels_);
	}
}
void* DepthBuffer::getTexels(texture::Descriptor2D& descriptor) {
	descriptor.width = size_.x;
	descriptor.height = size_.y;
	descriptor.format = texture::FLOAT_R_32;
	if(!texels_){
		texels_ = (float*)core::memory::globalAllocator()->allocate(size_.x*size_.y*sizeof(float),alignof(vec4f));
	}

	if(mode_ == kModeDepthFlat){
		memcpy(texels_,data_,size_.x*size_.y*sizeof(float));
	} else if(mode_ == kModeDepthPackedTiles){
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
	} else if(mode_ == kModeDepthPackedQuads){
		//memcpy(texels_,data_,size_.x*size_.y*sizeof(float));
		//return texels_;
		//Copy row by row
		float* dest = texels_;
		float* source = data_;
		for(int32 y = 0;y<size_.y;y+=2){
			auto src = source;
			for(int32 x = 0;x<size_.x;x+=2,src+=4){
				dest[x] = src[0];
				dest[x+1] = src[1];
				dest[x+size_.x] = src[2];
				dest[x+size_.x+1] = src[3];
			}
			//Goto the next two rows
			dest+=size_.x*2;
			source+=size_.x*2;
		}
	}

	return texels_;
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
	triangleBufferOffset= 0;
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
	return math::utils::gatherBoxVertices(vertices,min,max);
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
	__m128 screenCenterOffset = _mm_setr_ps(float(screenCenter.x),float(screenCenter.y),0,0);
	__m128 nearClip = _mm_setzero_ps();
	for(uint32 i = 0;i<8;++i){
		__m128 hv = _mm_load_ps((float*)(vertices + i));
		
		__m128 w  = _mm_shuffle_ps(hv,hv,_MM_SHUFFLE(3,3,3,3)); //get the w component
		__m128 z  = _mm_shuffle_ps(hv,hv,_MM_SHUFFLE(2,2,2,2));
		hv = _mm_div_ps(hv,w); //Project XYZW to clip space (divide by w)
		
		hv = _mm_mul_ps(hv,screenSpaceMul); //XY to screen space    [-width/2,-height/2 -> width/2,height/2]
		hv = _mm_add_ps(hv,screenCenterOffset);//XY to screen space [0,0 -> width,height]
		__m128 mNoNearClip = _mm_cmpge_ps(z, nearClip );

		//Set to all-0 if near-clipped
		hv = _mm_and_ps(hv, mNoNearClip);

		_mm_store_ps((float*)(vertices + i),hv);
	}
#else
	//TODO
	ScreenSpaceVertex* screenVerts= (ScreenSpaceVertex*)vertices;
	for(uint32 i =0;i<8;++i){
		vertices[i] = vertices[i] * (1.0f/vertices[i].w) ;
		auto v = vertices[i] * screenCenterMul;
		screenVerts[i].pos = vec2i(int32(v.x),int32(v.y))+screenCenter;
	}
#endif
}
static inline uint32 extractBoxQuads(ScreenSpaceQuad faces[6],vec4f vertices[8],uint32 aabbFrontFaceMask){
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

//Binning
static void gather4Simd(VecF32Soa dest[3],VecF32 vertices[12]){
	for(uint32 i = 0;i<3;++i){
		__m128 v0 = vertices[i].simd; //x0, y0, z0, w0
		__m128 v1 = vertices[3+i].simd;//x1, y1, z1, w1
		__m128 v2 = vertices[6+i].simd;//x2, y2, z2, w2
		__m128 v3 = vertices[9+i].simd;//x3, y3, z3, w3
		_MM_TRANSPOSE4_PS(v0, v1, v2, v3);
		dest[i].x = VecF32(v0);
		dest[i].y = VecF32(v1);
		dest[i].z = VecF32(v2);
		dest[i].w = VecF32(v3);
	}
}
void DepthBuffer::binTriangles4Simd(vec4f vertices[12],uint32 count) {
	enum { kNumLanes = 4 };

	VecF32Soa transformedPos[3];
	gather4Simd(transformedPos,(VecF32*)vertices);

	VecS32 vertexX[3],vertexY[3];
	VecF32 vertexZ[3];

	for(int i = 0;i<3;i++){
		//Convert the floating point coordinates to integer screen space coordinates.
		//NB: truncate
		vertexX[i] = ftoi(transformedPos[i].x);
		vertexY[i] = ftoi(transformedPos[i].y);

		vertexZ[i] = transformedPos[i].z;
	}

	//Compute triangle area.
	VecS32 area = (vertexX[1] - vertexX[0]) * (vertexY[2] - vertexY[0]) - (vertexX[0] - vertexX[2]) * (vertexY[0] - vertexY[1]);
	VecF32 oneOverArea = VecF32(1.0f)/itof(area);

	//Setup Z for interpolation
	vertexZ[1] = (vertexZ[1] - vertexZ[0]) * oneOverArea;
	vertexZ[2] = (vertexZ[2] - vertexZ[0]) * oneOverArea;

	//Find bounding box for the screen space triangle
	VecS32 zero = VecS32(0);
	VecS32 minX = vmax( vmin(vmin(vertexX[0],vertexX[1]),vertexX[2]), zero);
	VecS32 maxX = vmin( vmax(vmax(vertexX[0],vertexX[1]),vertexX[2]), VecS32(size_.x-1) );
	VecS32 minY = vmax( vmin(vmin(vertexY[0],vertexY[1]),vertexY[2]), zero);
	VecS32 maxY = vmin( vmax(vmax(vertexY[0],vertexY[1]),vertexY[2]), VecS32(size_.y-1) );

	uint32 numLanes = std::min(count,uint32(kNumLanes));
	for(uint32 i =0;i<numLanes;++i){
		//Skip triangle if the area is zero
		if(area.lane[i] <= 0) continue;

		float oneOverW[3];
		for(int j = 0;j<3;++j){
			oneOverW[j] = transformedPos[j].w.lane[i];
		}

		// Reject the triangle if any of its verts is behind the nearclip plane
		if(oneOverW[0] == 0.0f || oneOverW[1] == 0.0f || oneOverW[2] == 0.0f) continue;

		//Convert bounding box in terms of pixels to bounding box in terms of tiles.
		int32 tileMinX = minX.lane[i]/tileSize_.x;//std::max(minX.lane[i]/tileSize_.x,0);
		int32 tileMaxX = maxX.lane[i]/tileSize_.x;//std::min(maxX.lane[i]/tileSize_.x,tileCount_.x);
		int32 tileMinY = minY.lane[i]/tileSize_.y;//std::max(minY.lane[i]/tileSize_.y,0);
		int32 tileMaxY = maxY.lane[i]/tileSize_.y;//std::min(maxY.lane[i]/tileSize_.y,tileCount_.y);

		for(;tileMinY <= tileMaxY;tileMinY++){
			auto tileIndex = tileMinX + tileMinY*tileCount_.x;
		for(auto x = tileMinX; x<= tileMaxX; x++,tileIndex++){
			auto count = tileTriangleCount_[tileIndex];
			if(count >= kMaxTrianglesPerTile) continue;
			tileTriangleCount_[tileIndex]++;

			BinnedTriangle& triangle =*( triangleBins_ + count + x*kMaxTrianglesPerTile + tileMinY*tileCount_.x*kMaxTrianglesPerTile);
			triangle.v[0].x = vertexX[0].lane[i];
			triangle.v[0].y = vertexY[0].lane[i];
			triangle.v[1].x = vertexX[1].lane[i];
			triangle.v[1].y = vertexY[1].lane[i];
			triangle.v[2].x = vertexX[2].lane[i];
			triangle.v[2].y = vertexY[2].lane[i];
			triangle.z[0] = vertexZ[0].lane[i];
			triangle.z[1] = vertexZ[1].lane[i];
			triangle.z[2] = vertexZ[2].lane[i];
		} }
	}
}
void DepthBuffer::binTriangle(const vec4f& v0f,const vec4f& v1f,const vec4f& v2f) {
#ifdef ARPHEG_ARCH_X86
	if(triangleBufferOffset>=4){
		binTriangles4Simd(triangleBufferStorage,4);
		triangleBufferOffset = 0;
	}
	auto off = triangleBufferOffset*3;
	triangleBufferStorage[off] = v0f;
	triangleBufferStorage[off+1] = v1f;
	triangleBufferStorage[off+2] = v2f;
	triangleBufferOffset++;
#else
	vec2i v0;vec2i v1;vec2i v2;
	float z[3];

	//NB: truncate
	v0 = vec2i( int32(v0f.x),int32(v0f.y) );
	v1 = vec2i( int32(v1f.x),int32(v1f.y) );
	v2 = vec2i( int32(v2f.x),int32(v2f.y) );

	vec2i min,max;
	min.x = std::max(0,std::min(std::min(v0.x,v1.x),v2.x));
	min.y = std::max(0,std::min(std::min(v0.y,v1.y),v2.y));
	max.x = std::min(size_.x-1,std::max(std::max(v0.x,v1.x),v2.x));
	max.y = std::min(size_.y-1,std::max(std::max(v0.y,v1.y),v2.y));
	//Convert to tile coords
	min.x = min.x/tileSize_.x;//std::min(0,min.x / tileSize_.x);
	min.y = min.y/tileSize_.y;//std::min(0,min.y / tileSize_.y);
	max.x = max.x/tileSize_.x;//std::max(tileCount_.x-1,max.x / tileSize_.x);
	max.y = max.y/tileSize_.y;//std::max(tileCount_.y-1,max.y / tileSize_.y);
	
	//Calculate area
	auto area = (v1.x - v0.x) * (v2.y - v0.y) - (v0.x - v2.x) * (v0.y - v1.y);
	if(area <= 0) return;//skip if the area is zero
	float oneOverArea = 1.0f/float(area);

	//Setup Z
	z[0] = v0f.z;
	z[1] = (v1f.z-z[0])*oneOverArea;
	z[2] = (v2f.z-z[0])*oneOverArea;

	for(int32 y = min.y;y<=max.y;++y){
	for(int32 x = min.x;x<=max.x;++x){
		auto tileIndex = x + y*tileCount_.x;
		auto count = tileTriangleCount_[tileIndex];
		if(count >= kMaxTrianglesPerTile) continue;
		tileTriangleCount_[tileIndex]++;
		BinnedTriangle& triangle =*( triangleBins_ + count + x*kMaxTrianglesPerTile + y*tileCount_.x*kMaxTrianglesPerTile);

		triangle.v[0].x = v0.x;
		triangle.v[0].y = v0.y;
		triangle.v[1].x = v1.x;
		triangle.v[1].y = v1.y;
		triangle.v[2].x = v2.x;
		triangle.v[2].y = v2.y;
		triangle.z[0] = z[0];
		triangle.z[1] = z[1];
		triangle.z[2] = z[2];
	} }
#endif
}
void DepthBuffer::binQuad(ScreenSpaceQuad& quad) {
	//Find the quad bounds
#ifdef ARPHEG_ARCH_X86
	if(triangleBufferOffset>=3){
		binTriangles4Simd(triangleBufferStorage,4);
		triangleBufferOffset = 0;
	}
	auto off = triangleBufferOffset*3;
	triangleBufferStorage[off] = quad.v[0];
	triangleBufferStorage[off+1] = quad.v[1];
	triangleBufferStorage[off+2] = quad.v[2];
	triangleBufferStorage[off+3] = quad.v[2];
	triangleBufferStorage[off+4] = quad.v[3];
	triangleBufferStorage[off+5] = quad.v[0];
	triangleBufferOffset+=2;
#else
	binTriangle(quad.v[0],quad.v[1],quad.v[2]);
	binTriangle(quad.v[2],quad.v[3],quad.v[0]);
#endif
}
void DepthBuffer::binAABB(vec3f min,vec3f max) {
	//Transform the AABB vertices to Homogenous clip space
	vec4f vertices[8];
	gatherBoxVertices(vertices,min,max);
	transformBoxVertices(vertices,viewProjection_);
	boxVerticesToScreenVertices(vertices,clipSpaceToScreenSpaceMultiplier,center_);
	ScreenSpaceQuad faces[6];
	auto faceCount = extractBoxQuads(faces,vertices,aabbFrontFaceMask);
	for(uint32 i = 0;i<faceCount;++i)
		binQuad(faces[i]);
}

//Rasterization of tiles.
application::tasking::TaskID DepthBuffer::createRasterizationTasks() {
	//Flush unbinned triangles
#ifdef ARPHEG_ARCH_X86
	if(triangleBufferOffset>0){
		binTriangles4Simd(triangleBufferStorage,triangleBufferOffset);
		triangleBufferOffset = 0;
	}
#endif
	application::tasking::Task tasks[64];
	uint32 taskCount = tileCount_.x*tileCount_.y;
	assertRelease(taskCount < 64);
	struct Worker{
		static void work(application::tasking::Task* task){
			auto self = (DepthBuffer*)task->data;
			self->rasterizeTile(task->dataU32);
		}
	};
	for(uint32 i = 0;i<taskCount;++i){
		tasks[i].work = Worker::work;
		tasks[i].data = this;
		tasks[i].dataU32 = i;
	}
	return services::tasking()->addGroup(tasks,taskCount);
}
uint32 DepthBuffer::rasterizeTiles() {
	//Flush unbinned triangles
#ifdef ARPHEG_ARCH_X86
	if(triangleBufferOffset>0){
		binTriangles4Simd(triangleBufferStorage,triangleBufferOffset);
		triangleBufferOffset = 0;
	}
#endif
	uint32 count = 0;
	for(int32 i = 0;i<tileCount_.x*tileCount_.y;++i){
		 count+=tileTriangleCount_[i];
	}
	for(int32 y = 0;y<tileCount_.y;++y){
	for(int32 x = 0;x<tileCount_.x;++x){
		rasterizeTile(x,y,0);
	} }

	return count;
}
inline int32 orient2d(const vec2i a, const vec2i b, const vec2i c){
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
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
void DepthBuffer::rasterizeTile(uint32 id) {
	int32 x = int32(id%tileCount_.x);
	int32 y = int32(id/tileCount_.x);
	rasterizeTile(x,y,0);
}
void DepthBuffer::rasterizeTile(int32 x,int32 y,uint32 pass) {
	if(pass == 0){
		//init tile(clear depth).	
		//auto tilePixels = data_ + x*tileSize_.x*tileSize_.y + (y*tileSize_.x*tileSize_.y)*tileCount_.x;
		//clearDepth(tilePixels,tileSize_.x*tileSize_.y,1.0f);
	}
	if(mode_ == kModeDepthPackedQuads){
		rasterizeTile2x2(x,y,pass);
		return;
	}
	auto tileIndex = x + y*tileCount_.x;
	auto count = tileTriangleCount_[tileIndex];
	tileTriangleCount_[tileIndex] = 0;
	auto faces = triangleBins_ + x*kMaxTrianglesPerTile + y*tileCount_.x*kMaxTrianglesPerTile;
	vec2i tilePos(x*tileSize_.x,y*tileSize_.y);
	vec2i tileEnd(tilePos + tileSize_);
#ifdef ARPHEG_ARCH_X86
	enum { kNumLanes = 4 };

	//Flush denormals to zero
	//_mm_setcsr( _mm_getcsr() | 0x8040 );

	VecS32 colOffset(0, 1, 0, 1);
	VecS32 rowOffset(0, 0, 1, 1);

	//Process the 4 binned triangles at a time
	VecS32 vertexX[3];
	VecS32 vertexY[3];
	VecF32  vertexZ[4];
	VecS32 tileMinXSimd(tilePos.x);
	VecS32 tileMaxXSimd(tilePos.x+tileSize_.x-1);
	VecS32 tileMinYSimd(tilePos.y);
	VecS32 tileMaxYSimd(tilePos.y+tileSize_.y-1);

	for(uint32 i = 0;i<count;i += kNumLanes){

		uint32 numSimdTris = std::min(uint32(kNumLanes),count-i);
		auto f = faces+i;
		for(uint32 ii = 0;ii< numSimdTris;++ii){
			vertexX[0].lane[ii] = f[ii].v[0].x;
			vertexY[0].lane[ii] = f[ii].v[0].y;
			vertexX[1].lane[ii] = f[ii].v[1].x;
			vertexY[1].lane[ii] = f[ii].v[1].y;
			vertexX[2].lane[ii] = f[ii].v[2].x;
			vertexY[2].lane[ii] = f[ii].v[2].y;
			vertexZ[ii] = VecF32(f[ii].z[0],f[ii].z[1],f[ii].z[2],0.0f);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		VecS32 A0 = vertexY[1] - vertexY[2];
		VecS32 A1 = vertexY[2] - vertexY[0];
		VecS32 A2 = vertexY[0] - vertexY[1];

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		VecS32 B0 = vertexX[2] - vertexX[1];
		VecS32 B1 = vertexX[0] - vertexX[2];
		VecS32 B2 = vertexX[1] - vertexX[0];

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		VecS32 C0 = vertexX[1] * vertexY[2] - vertexX[2] * vertexY[1];
		VecS32 C1 = vertexX[2] * vertexY[0] - vertexX[0] * vertexY[2];
		VecS32 C2 = vertexX[0] * vertexY[1] - vertexX[1] * vertexY[0];

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		VecS32 minX = vmax(vmin(vmin(vertexX[0], vertexX[1]), vertexX[2]), tileMinXSimd); 
		VecS32 maxX   = vmin(vmax(vmax(vertexX[0], vertexX[1]), vertexX[2]), tileMaxXSimd);

		VecS32 minY = vmax(vmin(vmin(vertexY[0], vertexY[1]), vertexY[2]), tileMinYSimd); 
		VecS32 maxY   = vmin(vmax(vmax(vertexY[0], vertexY[1]), vertexY[2]), tileMaxYSimd);

		//Rasterize each triangle individually
		for(uint32 lane = 0;lane < numSimdTris;++lane){
			float zz[3] = { vertexZ[lane].lane[0],vertexZ[lane].lane[1],vertexZ[lane].lane[2] };

			int32 a0 = A0.lane[lane]; 
			int32 a1 = A1.lane[lane]; 
			int32 a2 = A2.lane[lane]; 
			int32 b0 = B0.lane[lane];
			int32 b1 = B1.lane[lane];
			int32 b2 = B2.lane[lane];

			int32 minx = minX.lane[lane];
			int32 maxx = maxX.lane[lane];
			int32 miny = minY.lane[lane];
			int32 maxy = maxY.lane[lane];

			auto w0_row = a0 * minx + b0 * miny + C0.lane[lane];
			auto w1_row = a1 * minx + b1 * miny + C1.lane[lane];
			auto w2_row = a2 * minx + b2 * miny + C2.lane[lane];

			float* tilePixels = data_ + tilePos.x*tileSize_.y + (tilePos.y*tileSize_.x)*tileCount_.x;
	
			int32 idx2  = minx-tilePos.x + (miny - tilePos.y)*tileSize_.x;
			int32 spanx = maxx-minx;

	
			for(int32 endIdx2 = idx2+(tileSize_.x)*(maxy-miny);idx2<=endIdx2;idx2+=tileSize_.x){
				auto w0 = w0_row;
				auto w1 = w1_row;
				auto w2 = w2_row;

				auto idx = idx2;
				for(int32 endIdx = idx+spanx;idx<=endIdx;++idx){
					auto mask = w0|w1|w2;
					if(mask >= 0){
						float betaf = float(w1);
						float gamaf = float(w2);
						float depth = zz[0] + betaf*zz[1] + gamaf*zz[2];

						auto d = tilePixels[idx];
						d = depth<d?depth:d;
						tilePixels[idx] = d;
					}

					w0+=a0;
					w1+=a1;
					w2+=a2;
				}
				w0_row += b0;
				w1_row += b1;
				w2_row += b2;
			}
		}
	}
#else
	for(uint32 i = 0;i<count;i ++){
		drawTriangle(faces[i],tilePos);
	}
#endif
}
//Rasterize 4 pixels at once
void DepthBuffer::rasterizeTile2x2(int32 x,int32 y,uint32 pass) {

	auto tileIndex = x + y*tileCount_.x;
	auto count = tileTriangleCount_[tileIndex];
	tileTriangleCount_[tileIndex] = 0;
	auto faces = triangleBins_ + x*kMaxTrianglesPerTile + y*tileCount_.x*kMaxTrianglesPerTile;
	vec2i tilePos(x*tileSize_.x,y*tileSize_.y);
	vec2i tileEnd(tilePos + tileSize_);
#ifdef ARPHEG_ARCH_X86
	enum { kNumLanes = 4 };

	//Flush denormals to zero
	_mm_setcsr( _mm_getcsr() | 0x8040 );

	VecS32 colOffset(0, 1, 0, 1);
	VecS32 rowOffset(0, 0, 1, 1);

	//Process the 4 binned triangles at a time
	VecS32 vertexX[3];
	VecS32 vertexY[3];
	VecF32  vertexZ[4];
	VecS32 tileMinXSimd(tilePos.x);
	VecS32 tileMaxXSimd(tilePos.x+tileSize_.x-2);
	VecS32 tileMinYSimd(tilePos.y);
	VecS32 tileMaxYSimd(tilePos.y+tileSize_.y-2);

	for(uint32 i = 0;i<count;i += kNumLanes){

		uint32 numSimdTris = std::min(uint32(kNumLanes),count-i);
		auto f = faces+i;
		for(uint32 ii = 0;ii< numSimdTris;++ii){
			vertexX[0].lane[ii] = f[ii].v[0].x;
			vertexY[0].lane[ii] = f[ii].v[0].y;
			vertexX[1].lane[ii] = f[ii].v[1].x;
			vertexY[1].lane[ii] = f[ii].v[1].y;
			vertexX[2].lane[ii] = f[ii].v[2].x;
			vertexY[2].lane[ii] = f[ii].v[2].y;
			vertexZ[ii] = VecF32(f[ii].z[0],f[ii].z[1],f[ii].z[2],0.0f);
		}

		// Fab(x, y) =     Ax       +       By     +      C              = 0
		// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
		// Compute A = (ya - yb) for the 3 line segments that make up each triangle
		VecS32 A0 = vertexY[1] - vertexY[2];
		VecS32 A1 = vertexY[2] - vertexY[0];
		VecS32 A2 = vertexY[0] - vertexY[1];

		// Compute B = (xb - xa) for the 3 line segments that make up each triangle
		VecS32 B0 = vertexX[2] - vertexX[1];
		VecS32 B1 = vertexX[0] - vertexX[2];
		VecS32 B2 = vertexX[1] - vertexX[0];

		// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
		VecS32 C0 = vertexX[1] * vertexY[2] - vertexX[2] * vertexY[1];
		VecS32 C1 = vertexX[2] * vertexY[0] - vertexX[0] * vertexY[2];
		VecS32 C2 = vertexX[0] * vertexY[1] - vertexX[1] * vertexY[0];

		// Use bounding box traversal strategy to determine which pixels to rasterize 
		VecS32 minX = vmax(vmin(vmin(vertexX[0], vertexX[1]), vertexX[2]), tileMinXSimd) & VecS32(~1);
		VecS32 maxX   = vmin(vmax(vmax(vertexX[0], vertexX[1]), vertexX[2]), tileMaxXSimd);

		VecS32 minY = vmax(vmin(vmin(vertexY[0], vertexY[1]), vertexY[2]), tileMinYSimd) & VecS32(~1);
		VecS32 maxY = vmin(vmax(vmax(vertexY[0], vertexY[1]), vertexY[2]), tileMaxYSimd);

		//Rasterize each triangle individually
		for(uint32 lane = 0;lane < numSimdTris;++lane){
			//Rasterize in 2x2 quads.
			VecF32 zz[3];
			zz[0] = VecF32(vertexZ[lane].lane[0]);
			zz[1] = VecF32(vertexZ[lane].lane[1]);
			zz[2] = VecF32(vertexZ[lane].lane[2]);

			VecS32 a0(A0.lane[lane]);
			VecS32 a1(A1.lane[lane]);
			VecS32 a2(A2.lane[lane]);
			VecS32 b0(B0.lane[lane]);
			VecS32 b1(B1.lane[lane]);
			VecS32 b2(B2.lane[lane]);

			int32 minx = minX.lane[lane];
			int32 maxx = maxX.lane[lane];
			int32 miny = minY.lane[lane];
			int32 maxy = maxY.lane[lane];

			VecS32 col = VecS32(minx) + colOffset;
			VecS32 row = VecS32(miny) + rowOffset;
			auto rowIdx = miny*size_.x + 2 * minx;
			VecS32 w0_row  = a0 * col + b0 * row + VecS32(C0.lane[lane]);
			VecS32 w1_row  = a1 * col + b1 * row + VecS32(C1.lane[lane]);
			VecS32 w2_row  = a2 * col + b2 * row + VecS32(C2.lane[lane]);

			//Multiply each weight by two(rasterize 2x2 quad at once).
			a0 = shiftl<1>(a0);
			a1 = shiftl<1>(a1);
			a2 = shiftl<1>(a2);
			b0 = shiftl<1>(b0);
			b1 = shiftl<1>(b1);
			b2 = shiftl<1>(b2);

			VecF32 zInc = itof(a1)*zz[1] + itof(a2)*zz[2];
	
			for(int32 y = miny;y<=maxy;y+=2,rowIdx += 2 * size_.x){
				auto w0 = w0_row;
				auto w1 = w1_row;
				auto w2 = w2_row;

				VecF32 depth = zz[0] + itof(w1)*zz[1] + itof(w2)*zz[2];
				auto idx = rowIdx;
				
				for(int32 x = minx;x<=maxx;x+=2,idx+=4){
					auto mask = w0|w1|w2;
					VecF32 previousDepth = VecF32::load(data_+idx);
					VecF32 mergedDepth = vmin(depth,previousDepth);
					previousDepth = select(mergedDepth,previousDepth,mask);
					previousDepth.store(data_+idx);
	
					w0+=a0;
					w1+=a1;
					w2+=a2;
					depth+=zInc;
				}
				w0_row += b0;
				w1_row += b1;
				w2_row += b2;
			}
		}
	}
#endif
}

//Returns true if the triangle is visible
bool DepthBuffer::testTriangle2x2(const vec4f& v0,const vec4f& v1,const vec4f& v2){
	VecS32 colOffset(0, 1, 0, 1);
	VecS32 rowOffset(0, 0, 1, 1);

	vec2i vertex[3];
	vertex[0] = vec2i(int32(v0.x),int32(v0.y));
	vertex[1] = vec2i(int32(v1.x),int32(v1.y));
	vertex[2] = vec2i(int32(v2.x),int32(v2.y));

	// Reject the triangle if any of its verts is behind the nearclip plane
	if(v0.w == 0.0f || v1.w == 0.0f || v2.w == 0.0f) return true;

	float minZ = std::min(v0.z,std::min(v1.z,v2.z));
	VecF32 fixedDepth(minZ);

	// Fab(x, y) =     Ax       +       By     +      C              = 0
	// Fab(x, y) = (ya - yb)x   +   (xb - xa)y + (xa * yb - xb * ya) = 0
	// Compute A = (ya - yb) for the 3 line segments that make up each triangle
	auto A0 = vertex[1].y - vertex[2].y;
	auto A1 = vertex[2].y - vertex[0].y;
	auto A2 = vertex[0].y - vertex[1].y;

	// Compute B = (xb - xa) for the 3 line segments that make up each triangle
	auto B0 = vertex[2].x - vertex[1].x;
	auto B1 = vertex[0].x - vertex[2].x;
	auto B2 = vertex[1].x - vertex[0].x;

	// Compute C = (xa * yb - xb * ya) for the 3 line segments that make up each triangle
	auto C0 = vertex[1].x * vertex[2].y - vertex[2].x * vertex[1].y;
	auto C1 = vertex[2].x * vertex[0].y - vertex[0].x * vertex[2].y;
	auto C2 = vertex[0].x * vertex[1].y - vertex[1].x * vertex[0].y;

	// Use bounding box traversal strategy to determine which pixels to rasterize 
	auto minx = std::max(std::min(std::min(vertex[0].x,vertex[1].x),vertex[2].x),0) & (~1);
	auto maxx = std::min(std::max(std::max(vertex[0].x,vertex[1].x),vertex[2].x),size_.x-2);
	auto miny = std::max(std::min(std::min(vertex[0].y,vertex[1].y),vertex[2].y),0) & (~1);
	auto maxy = std::min(std::max(std::max(vertex[0].y,vertex[1].y),vertex[2].y),size_.y-2);

	VecS32 a0(A0);
	VecS32 a1(A1);
	VecS32 a2(A2);
	VecS32 b0(B0);
	VecS32 b1(B1);
	VecS32 b2(B2);

	VecS32 col = VecS32(minx) + colOffset;
	VecS32 row = VecS32(miny) + rowOffset;
	auto rowIdx = miny*size_.x + 2 * minx;
	VecS32 w0_row  = a0 * col + b0 * row + VecS32(C0);
	VecS32 w1_row  = a1 * col + b1 * row + VecS32(C1);
	VecS32 w2_row  = a2 * col + b2 * row + VecS32(C2);

	//Multiply each weight by two(rasterize 2x2 quad at once).
	a0 = shiftl<1>(a0);
	a1 = shiftl<1>(a1);
	a2 = shiftl<1>(a2);
	b0 = shiftl<1>(b0);
	b1 = shiftl<1>(b1);
	b2 = shiftl<1>(b2);

	for(int32 y = miny;y<=maxy;y+=2,rowIdx += 2 * size_.x){
		auto w0 = w0_row;
		auto w1 = w1_row;
		auto w2 = w2_row;

		auto idx = rowIdx;
				
		for(int32 x = minx;x<=maxx;x+=2,idx+=4){
			auto mask = w0|w1|w2;
			auto masks = _mm_movemask_ps(bits2float(mask).simd);
			if(masks != 0xF){
				VecF32 previousDepth = VecF32::load(data_+idx);
				auto cmpMask = ((~masks)&0xF)& _mm_movemask_ps(cmple(fixedDepth,previousDepth).simd);
				if(cmpMask){
					return true;
				}
			}
			
			w0+=a0;
			w1+=a1;
			w2+=a2;
		}
		w0_row += b0;
		w1_row += b1;
		w2_row += b2;
	}
	return false;
}

bool DepthBuffer::testAABB(vec3f min,vec3f max){
	vec4f vertices[8];
	gatherBoxVertices(vertices,min,max);
	transformBoxVertices(vertices,viewProjection_);
	boxVerticesToScreenVertices(vertices,clipSpaceToScreenSpaceMultiplier,center_);
	ScreenSpaceQuad faces[6];
	auto faceCount = extractBoxQuads(faces,vertices,aabbFrontFaceMask);
	for(uint32 i = 0;i<faceCount;++i){
		if(testTriangle2x2(faces[i].v[0],faces[i].v[1],faces[i].v[2])) return true;
		if(testTriangle2x2(faces[i].v[2],faces[i].v[3],faces[i].v[0])) return true;
	}
	return false;

	//Transform the AABB vertices to Homogenous clip space
	//vec4f vertices[8];
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
	screenMin.x &= (~1);screenMin.y &= (~1);
	vec2i screenMax = vec2i(int32(clipMax.x),int32(clipMax.y))+center_;
	
	auto rowIdx = screenMin.y*size_.x + 2 * screenMin.x;

	float minZ = vertices[0].z;
	for(uint32 i = 1;i< 8;i++){
		minZ = std::min(minZ,vertices[i].z);
	}
	VecF32 flatDepth(minZ);

	//Iterate over the pixels
	for(;screenMin.y < screenMax.y;screenMin.y+=2,rowIdx += 2 * size_.x){
		auto idx = rowIdx;
	for(int32 x = screenMin.x;x<screenMax.x;x+=2,idx+=4){
		//Fetch the distance value for the current pixel.
		auto depth = VecF32::load(data_ + idx);
		vec3f rayo,rayd;
		getRay(rayo,rayd,x,screenMin.y);
		float dist;
		if(!rayAABBIntersect(min,max,rayo,rayd,dist)) continue;
		dist = ((dist-znear_)/zfar_ ) * 2.0f - 1.0f;
		VecF32 flatDepth(dist);
		flatDepth.store(data_+idx);
		auto mask = _mm_movemask_ps(cmple(flatDepth,depth).simd);
		//if(mask != 0)//{
			//return true; //Visible
		//}
			
		//
		//

		//Compute the distance to the aabb (raytrace)
		//vec3f rayo,rayd;
		//getRay(rayo,rayd,x,screenMin.y);
		//float dist;
		
		//Convert the distance from view space to depth space [-1,1]
		//dist = ((dist-znear_)/zfar_ ) * 2.0f - 1.0f;
		//Compare the values.
		//if(dist <= depth){
			//return true;
		//	data_[idx] = dist;
		//}
	} }
	return false;
}

} }