// Implements frustum culling of spheres and aabbs
// Used to filter all the renderable entities in a scene into arrays of visible entities with visibility masks.
#pragma once

#include "types.h"
#include "../scene/types.h"

namespace rendering {
namespace frustumCulling {

typedef uint32 Mask;
 
struct Frustum    {
	vec4f planes[6];

	inline Frustum() {}
	//Extracts the frustum planes from a given camera
	Frustum(const Camera& camera);
};
struct FrustumSoA {
	vec4f planes0_4_x;
	vec4f planes0_4_w;
	vec4f planes0_4_y;
	vec4f planes0_4_z;
	vec4f planes4_6_x;
	vec4f planes4_6_w;
	vec4f planes4_6_y;
	vec4f planes4_6_z;

	void extract(const Frustum& frustum);
};
 
struct Culler {
    enum { kMaxFrustums = 32 };
	scene::rendering::Service* scene;
    uint32 frustumCount;
    FrustumSoA frustums[kMaxFrustums];
	Frustum frustumsAoS[kMaxFrustums];
 
	Culler(scene::rendering::Service* scene);
	Mask addFrustum(const Frustum& frustum);
};

void cullSpheres(Culler* culler,const scene::rendering::EntityGridCell* cell);
void cullAABBs(Culler* culler,const scene::rendering::EntityGridCell* cell);

} }
