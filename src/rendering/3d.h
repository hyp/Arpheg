//3D world rendering - meshes, submeshes, skinned meshes.

#pragma once

#include "types.h"
#include "../scene/types.h"
#include "../data/types.h"

namespace rendering {

class DirectMeshRenderer: public scene::events::IMeshRenderer {
public:
	uint32 state_;
	Camera camera_;
	



	DirectMeshRenderer();
	void servicePreStep();
	
	void draw(const scene::events::Draw& ev,const data::SubMesh* mesh,const data::Material* material); 
	void draw(const scene::events::Draw& ev,const data::SubMesh* mesh,const data::Material* material,const data::Transformation3D* transforms,uint32 count);
	void bind(const Camera& camera);
	void bind(data::Pipeline* pipeline,data::Pipeline* anim);
private:
	rendering::Pipeline staticMesh_;Pipeline::Constant* mvp;
	rendering::Pipeline animatedMesh_;Pipeline::Constant* amvp;
	void meshInit();
};

}
