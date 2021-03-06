#pragma once
#include "../core/math.h"

namespace collisions {

	//Axis aligned bouding box
	struct AABox3D {
		vec3f min,max;
	};
	struct Frustum {
		vec4f planes[6];

		inline Frustum(){}
		Frustum(const mat44f& viewProjection);
	};
	bool  overlaps(const Frustum& frustum,const AABox3D& b);


	bool  overlaps(const AABox3D& a,const AABox3D& b);

	bool  AABBoverlapsAABB (const AABB& a,const AABB& b);

	//The projection vector is used as an offset to push out the a AABB
	bool  AABBoverlapsAABB(const AABB& a,const AABB& b,vec2f& projection);
	bool  pointInAABB(const AABB& a,const vec2f p);

}
