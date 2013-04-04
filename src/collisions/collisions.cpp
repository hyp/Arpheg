#include "collisions.h"

namespace collisions {

Frustum::Frustum(const mat44f& viewProjection) {
	const float* clip = &viewProjection.a.x;

	// Extract the RIGHT clipping plane
	planes[0] = vec4f(clip[3]-clip[0],clip[7]-clip[4],clip[11]-clip[8],clip[15]-clip[12]).normalize();
	// Extract the LEFT clipping plane
	planes[1] = vec4f(clip[ 3] + clip[ 0],clip[ 7] + clip[ 4],clip[11] + clip[ 8],clip[15] + clip[12]).normalize();

	// Extract the BOTTOM clipping plane
	planes[2] = vec4f(clip[3]+clip[1],clip[7]+clip[5],clip[11]+clip[9],clip[15]+clip[13]).normalize();
	// Extract the TOP clipping plane
	planes[3] = vec4f(clip[3]-clip[1],clip[7]-clip[5],clip[11]-clip[9],clip[15]-clip[13]).normalize();

	// Extract the FAR clipping plane
	planes[4] = vec4f(clip[3]-clip[2],clip[7]-clip[6],clip[11]-clip[10],clip[15]-clip[14]).normalize();
	// Extract the NEAR clipping plane
	planes[5] = vec4f(clip[3]+clip[2],clip[7]+clip[6],clip[11]+clip[10],clip[15]+clip[14]).normalize();
}

inline float planeDistance(const vec4f& plane,const vec3f point){
	return plane.x*point.x+plane.y*point.y+plane.z*point.z+plane.w;
}
inline void pnVertex(const vec4f& plane,vec3f& min,vec3f& max){
	if(plane.x >= 0){
		float t = min.x; min.x = max.x; max.x = t;
	}
	if(plane.y >= 0){
		float t = min.y; min.y = max.y; max.y = t;
	}
	if(plane.z >= 0){
		float t = min.z; min.z = max.z; max.z = t;
	}
}

bool  overlaps(const Frustum& frustum,const AABox3D& b) {
	for(int i = 0; i < 6; i++ ) {
		vec3f p = b.min,n = b.max;
		pnVertex(frustum.planes[i],p,n);

		if(planeDistance(frustum.planes[i],p) < 0.0f) return false;
	}
	return true;
}

bool  overlaps(const AABox3D& a,const AABox3D& b){
	//overlap on the x axis?
	if(a.min.x > b.min.x){
		if(b.max.x <= a.min.x) return false;
	}
	else {
		if(a.max.x <= b.min.x) return false;
	}
	//overlap on the y axis?
	if(a.min.y > b.min.y){
		if(b.max.y <= a.min.y) return false;
	}
	else {
		if(a.max.y <= b.min.y) return false;
	}
	//overlap on the z axis?
	if(a.min.z > b.min.z){
		if(b.max.z <= a.min.z) return false;
	}
	else {
		if(a.max.z <= b.min.z) return false;
	}
	return true;
}

bool  AABBoverlapsAABB (const AABB& a,const AABB& b){
	//overlap on the x axis?
	if(a.min.x > b.min.x){
		if(b.max.x <= a.min.x) return false;
	}
	else {
		if(a.max.x <= b.min.x) return false;
	}
	//overlap on the y axis?
	if(a.min.y > b.min.y){
		if(b.max.y <= a.min.y) return false;
	}
	else {
		if(a.max.y <= b.min.y) return false;
	}
	return true;
}
bool AABBoverlapsAABB (const AABB& a,const AABB& b,vec2f& projection){
	float ox,oy;
	//overlap on the x axis?
	if(a.min.x > b.min.x){
		if(b.max.x <= a.min.x) return false;
		ox = b.max.x - a.min.x;
	}
	else {
		if(a.max.x <= b.min.x) return false;;
		ox = b.min.x - a.max.x;
	}
	//overlap on the y axis?
	if(a.min.y > b.min.y){
		if(b.max.y <= a.min.y) return false;
		oy = b.max.y - a.min.y;
	}
	else {
		if(a.max.y <= b.min.y) return false;
		oy = b.min.y - a.max.y;
	}
	if(ox < oy){
		projection.x = ox;
		projection.y = 0.0f;
	}
	else {
		projection.x = 0.0f;
		projection.y = oy;
	}
	return true;
}
bool  pointInAABB(const AABB& a,const vec2f p){
	return p.x >= a.min.x && p.y >= a.min.y && p.x < a.max.x && p.y < a.max.y;
}

}
