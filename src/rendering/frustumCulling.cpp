#include "../core/assert.h"
#include "../scene/rendering.h"
#include "frustumCulling.h"

namespace rendering {
namespace frustumCulling {

static inline vec4f planeNormalize(const vec4f& x){
	auto norm = 1.0f/x.length3();
	return x*norm;
}

//Extracts the frustum planes given a camera
Frustum::Frustum(const Camera& camera) {
	const float* clip = &camera.projectionView.a.x;

	// Extract the RIGHT clipping plane
	planes[0] = planeNormalize(vec4f(clip[3]-clip[0],clip[7]-clip[4],clip[11]-clip[8],clip[15]-clip[12]));
	// Extract the LEFT clipping plane
	planes[1] = planeNormalize(vec4f(clip[ 3] + clip[ 0],clip[ 7] + clip[ 4],clip[11] + clip[ 8],clip[15] + clip[12]));

	// Extract the BOTTOM clipping plane
	planes[2] = planeNormalize(vec4f(clip[3]+clip[1],clip[7]+clip[5],clip[11]+clip[9],clip[15]+clip[13]));
	// Extract the TOP clipping plane
	planes[3] = planeNormalize(vec4f(clip[3]-clip[1],clip[7]-clip[5],clip[11]-clip[9],clip[15]-clip[13]));

	// Extract the FAR clipping plane
	planes[4] = planeNormalize(vec4f(clip[3]-clip[2],clip[7]-clip[6],clip[11]-clip[10],clip[15]-clip[14]));
	// Extract the NEAR clipping plane
	planes[5] = planeNormalize(vec4f(clip[3]+clip[2],clip[7]+clip[6],clip[11]+clip[10],clip[15]+clip[14]));
}

//AoS -> SoA
void FrustumSoA::extract(const Frustum& frustum) {
#ifdef ARPHEG_ARCH_X86
	__m128 v[4];
	//planes 0 - 4
	for(int i =0;i<4;++i) v[i] = frustum.planes[i].xyzw;
	_MM_TRANSPOSE4_PS(v[0],v[1],v[2],v[3]);
    planes0_4_x = v[0];planes0_4_y = v[1];
	planes0_4_z = v[2];planes0_4_w = v[3];
	
	//Planes 4-6
	v[0] = v[2] = frustum.planes[4].xyzw;
	v[1] = v[3] = frustum.planes[5].xyzw;
	_MM_TRANSPOSE4_PS(v[0],v[1],v[2],v[3]);
    planes4_6_x = v[0];planes4_6_y = v[1];
	planes4_6_z = v[2];planes4_6_w = v[3];
#endif
}
static inline Mask intersect(const FrustumSoA& frustum,const vec4f& sphere){
#ifdef ARPHEG_ARCH_X86
	 __m128 sp = sphere.xyzw;
	 __m128 pos_xxxx = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(0,0,0,0)); 
	 __m128 pos_yyyy = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(1,1,1,1)); 
	 __m128 pos_zzzz = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(2,2,2,2)); 
	 __m128 pos_rrrr = _mm_sub_ps(_mm_set_ps1(1.0f),_mm_shuffle_ps(sp,sp,_MM_SHUFFLE(3,3,3,3))); // - r

	 __m128 dotA_0123 = vec4f::fma(pos_xxxx,frustum.planes0_4_x.xyzw,frustum.planes0_4_w.xyzw);
	 dotA_0123 = vec4f::fma(pos_yyyy,frustum.planes0_4_y.xyzw,dotA_0123);
	 dotA_0123 = vec4f::fma(pos_zzzz,frustum.planes0_4_z.xyzw,dotA_0123);

	 //NB: this is a waste of half register.
	 __m128 dotA_45 = vec4f::fma(pos_xxxx,frustum.planes4_6_x.xyzw,frustum.planes4_6_w.xyzw);
	 dotA_45 = vec4f::fma(pos_yyyy,frustum.planes4_6_y.xyzw,dotA_45);
	 dotA_45 = vec4f::fma(pos_zzzz,frustum.planes4_6_z.xyzw,dotA_45);

	 //comparison mask = distance x 6 < - r x 6
	 dotA_0123 = _mm_cmplt_ps(dotA_0123, pos_rrrr);
	 dotA_45   = _mm_cmplt_ps(dotA_45, pos_rrrr);
	 dotA_0123 = _mm_or_ps(dotA_0123,dotA_45);
	 
	 //If any masks are 0xFFFF_FFFF return false.
	 uint32 mask = uint32(_mm_movemask_ps(dotA_0123)); //mask is zero if r <= for all planes.
	 //Is there a better way?
	 return mask == 0? 0xFFffFFff : 0;
#else
	auto pos_xxxx = sphere.xxxx();
	auto pos_yyyy = sphere.yyyy();
	auto pos_zzzz = sphere.zzzz();
	auto pos_rrrr = sphere.wwww();

	auto dotA_0123 = vec4f::fma(pos_xxxx,frustum.planes0_4_x,frustum.planes0_4_w);
	dotA_0123 = vec4f::fma(pos_yyyy,frustum.planes0_4_y,dotA_0123);
	dotA_0123 = vec4f::fma(pos_zzzz,frustum.planes0_4_z,dotA_0123);
	//NB: this is a waste of half a register.
	auto dotA_45 = vec4f::fma(pos_xxxx,frustum.planes4_6_x,frustum.planes4_6_w);
	dotA_45 = vec4f::fma(pos_yyyy,frustum.planes4_6_y,dotA_45);
	dotA_45 = vec4f::fma(pos_zzzz,frustum.planes4_6_z,dotA_45);

	//TODO
#endif
	return 0xFFffFFff;
}
static inline Mask intersect(const Frustum& frustum,const vec4f& sphere){
	for(int i =0;i<6;++i)
		if((frustum.planes[i].dot3(sphere) + frustum.planes[i].w) < -sphere.w) return 0;
	return 0xFFffFFff;
}

//Use the PN vertex AABB test.
static inline Mask intersect(const Frustum& frustum,const vec4f& aabbMin,const vec4f& aabbMax) {
	auto p = aabbMin,n = aabbMax;
	for(int i = 0; i < 6; i++ ) {
		auto p = aabbMin;
		if(frustum.planes[i].x >= 0.f) p.x = aabbMax.x;
		if(frustum.planes[i].y >= 0.f) p.y = aabbMax.x;
		if(frustum.planes[i].z >= 0.f) p.z = aabbMax.x;
		
		if((frustum.planes[i].dot3(p) + frustum.planes[i].w) < 0.0f) return 0;
	}
	return 0xFFffFFff;
}
static inline Mask intersect(const FrustumSoA& frustum,const vec4f& aabbMin,const vec4f& aabbMax){
#ifdef ARPHEG_ARCH_X86
	auto sp = aabbMin.xyzw;
	 __m128 aabbMin_xxxx = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(0,0,0,0)); 
	 __m128 aabbMin_yyyy = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(1,1,1,1)); 
	 __m128 aabbMin_zzzz = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(2,2,2,2)); 
	sp = aabbMax.xyzw;
	 __m128 aabbMax_xxxx = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(0,0,0,0)); 
	 __m128 aabbMax_yyyy = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(1,1,1,1)); 
	 __m128 aabbMax_zzzz = _mm_shuffle_ps(sp,sp,_MM_SHUFFLE(2,2,2,2)); 
	 __m128 zero = vec4f::zero().xyzw;

	 //P vertices for 6 planes.
	 __m128 p04x,p04y,p04z,p46x,p46y,p46z,mask;
	 
	 mask =_mm_cmpge_ps(frustum.planes0_4_x.xyzw,zero);
	 p04x = _mm_or_ps(_mm_andnot_ps(mask,aabbMin_xxxx),_mm_and_ps(mask,aabbMax_xxxx));//NB: Blend instruction(SSE4) can be utilized instead.
	 mask =_mm_cmpge_ps(frustum.planes0_4_y.xyzw,zero);
	 p04y = _mm_or_ps(_mm_andnot_ps(mask,aabbMin_yyyy),_mm_and_ps(mask,aabbMax_yyyy));
	 mask =_mm_cmpge_ps(frustum.planes0_4_z.xyzw,zero);
	 p04z = _mm_or_ps(_mm_andnot_ps(mask,aabbMin_zzzz),_mm_and_ps(mask,aabbMax_zzzz));
	 
	 __m128 dotA_0123 = vec4f::fma(p04x,frustum.planes0_4_x.xyzw,frustum.planes0_4_w.xyzw);
	 dotA_0123 = vec4f::fma(p04y,frustum.planes0_4_y.xyzw,dotA_0123);
	 dotA_0123 = vec4f::fma(p04z,frustum.planes0_4_z.xyzw,dotA_0123);

	 //NB: this is a waste of half register.
	 mask =_mm_cmpge_ps(frustum.planes4_6_x.xyzw,zero);
	 p46x = _mm_or_ps(_mm_andnot_ps(mask,aabbMin_xxxx),_mm_and_ps(mask,aabbMax_xxxx));
	 mask =_mm_cmpge_ps(frustum.planes4_6_y.xyzw,zero);
	 p46y = _mm_or_ps(_mm_andnot_ps(mask,aabbMin_yyyy),_mm_and_ps(mask,aabbMax_yyyy));
	 mask =_mm_cmpge_ps(frustum.planes4_6_z.xyzw,zero);
	 p46z = _mm_or_ps(_mm_andnot_ps(mask,aabbMin_zzzz),_mm_and_ps(mask,aabbMax_zzzz));

	 __m128 dotA_45 = vec4f::fma(p46x,frustum.planes4_6_x.xyzw,frustum.planes4_6_w.xyzw);
	 dotA_45 = vec4f::fma(p46y,frustum.planes4_6_y.xyzw,dotA_45);
	 dotA_45 = vec4f::fma(p46z,frustum.planes4_6_z.xyzw,dotA_45);

	 //comparison mask = distance x 6 < 0 x 6
	 dotA_0123 = _mm_cmplt_ps(dotA_0123, zero);
	 dotA_45   = _mm_cmplt_ps(dotA_45, zero);
	 dotA_0123 = _mm_or_ps(dotA_0123,dotA_45);
	 
	 //If any masks are 0xFFFF_FFFF return false.
	 uint32 m = uint32(_mm_movemask_ps(dotA_0123)); //mask is zero if r <= for all planes.
	 //Is there a better way?
	 return m == 0? 0xFFffFFff : 0;
#else
	return 0xFFffFFff;
#endif
}

Culler::Culler(scene::rendering::Service* sce) : frustumCount(0),scene(sce) {}
Mask Culler::addFrustum(const Frustum& frustum) {
	assert(frustumCount < kMaxFrustums);
	frustums[frustumCount].extract(frustum);
	frustumsAoS[frustumCount] = frustum;
	frustumCount++;
	return 1<<(frustumCount-1);
}

void cullSpheres(Culler* culler,const scene::rendering::EntityGridCell* cell){
    using namespace scene::rendering;
    
    Mask masks[EntityGridCell::kMaxBlockEntities];
    for(uint32 j =0;j < EntityGridCell::kMaxBlocks;++j){
        const EntityGridCell::Block& block = cell->sphereFrustumCullers.blocks[j];         
        //Cull
        for(uint32 k = 0,fmask = 1;k < culler->frustumCount;++k,fmask<<=1){
			const FrustumSoA& frustum = culler->frustums[k];
            for(uint32 i =0;i < uint32(cell->sphereFrustumCullers.count[j]);++i){
                auto inside = intersect(frustum,block.positions[i]);
				//auto inside2 = intersect(culler->frustumsAoS[j],block.positions[i]);
                masks[i] |= fmask & inside;
            }
        }   
        //Filter
		auto scene = culler->scene;
        for(uint32 i = 0;i < uint32(cell->sphereFrustumCullers.count[j]);++i){
            if(masks[i] == 0) continue;
			scene->markVisibleEntity(cell,masks[i],block.entities[i]);
        }
    }
}
void cullAABBs  (Culler* culler,const scene::rendering::EntityGridCell* cell) {
    using namespace scene::rendering;
    
    Mask masks[EntityGridCell::kMaxBlockEntities];
	for(uint32 j =0;j < EntityGridCell::kMaxBlocks;++j){
		const EntityGridCell::BoxBlock& block = cell->boxFrustumCullers.blocks[j];   
		//Cull
        for(uint32 k = 0,fmask = 1;k < culler->frustumCount;++k,fmask<<=1){
			const FrustumSoA& frustum = culler->frustums[k];
            for(uint32 i =0;i < uint32(cell->boxFrustumCullers.count[j]);++i){
                auto inside = intersect(frustum,block.positions[i].min,block.positions[i].max);
                masks[i] |= fmask & inside;
            }
        }
        //Filter
		auto scene = culler->scene;
        for(uint32 i = 0;i < uint32(cell->boxFrustumCullers.count[j]);++i){
            if(masks[i] == 0) continue;
			scene->markVisibleEntity(cell,masks[i],block.entities[i]);
        }
	}
}

} }