#include "../core/assert.h"
#include "../services.h"
#include "animation.h"
#include "3d.h"

namespace rendering {


static void drawSubmesh(const data::Material* material,const data::SubMesh* submesh) {
	
	
	auto renderer = services::rendering();
	if(material){
		uint32 slots[::data::Material::kMaxTextures] = {0};
		for(uint32 i = 0;i<material->textureCount();++i)
			renderer->bind(material->textures()[i],i);
		renderer->bind(rendering::Pipeline::Constant("texture"),slots);
	}
	renderer->bind(submesh->mesh(),submesh->primitiveKind(),submesh->indexSize());
	renderer->drawIndexed(submesh->primitiveOffset(),submesh->primitiveCount());
}

DirectMeshRenderer::DirectMeshRenderer() : mvp(nullptr) {
	state_ = 0;
}
void DirectMeshRenderer::servicePreStep() {
	state_ = 0;
}
void DirectMeshRenderer::bind(const Camera& camera) {
	camera_ = camera;
}
void DirectMeshRenderer::bind(data::Pipeline* pipeline,data::Pipeline* anim) {
	staticMesh_ = pipeline->pipeline();mvp = &pipeline->modelViewProjectionConstant();
	animatedMesh_ = anim->pipeline();amvp = &anim->modelViewProjectionConstant();	
}
void DirectMeshRenderer::meshInit() {
	///for(uint32 i = 0;i<data::Material::kMaxTextures){
	///
	//}
}
/*
void DirectMeshRenderer::draw(const scene::events::Draw& ev,const scene::components::StaticMesh& mesh) {
	assert(mvp);

	auto subs = mesh.submeshes();
	for(size_t i = 0;i<mesh.submeshCount();++i){
		services::rendering()->bind(*mvp,camera_.calculateMvp(mat44f::identity()));
		drawSubmesh(subs[i].material,subs[i].mesh);
	}
}

void DirectMeshRenderer::draw(const scene::events::Draw& ev,const scene::components::SkinnedMesh& mesh){
	assert(mvp);

	animation::JointTransformation3D jointTransformations[data::Mesh::kMaxBones];
	
	rendering::Pipeline::Constant c("boneMatrices");

	auto subs = mesh.submeshes();
	for(size_t i = 0;i<mesh.submeshCount();++i){
		rendering::animation::Animator::bindSkeleton(subs[i].mesh,mesh.skeletonNodeCount_,mesh.skeletonTransformations(),jointTransformations);
		services::rendering()->bind(c,(void*)jointTransformations);

		services::rendering()->bind(*mvp,camera_.calculateMvp(mat44f::identity()));
		drawSubmesh(subs[i].material,subs[i].mesh);
	}
}*/
void DirectMeshRenderer::draw(const scene::events::Draw& ev,const data::SubMesh* mesh,const data::Material* material) {
	services::rendering()->bind(staticMesh_);
	services::rendering()->bind(*mvp,camera_.calculateMvp(ev.entityGlobalTransformation));
	drawSubmesh(material,mesh);
}
void DirectMeshRenderer::draw(const scene::events::Draw& ev,const data::SubMesh* mesh,const data::Material* material,const data::Transformation3D* transforms,uint32 count) {
	services::rendering()->bind(animatedMesh_);
	services::rendering()->bind(*amvp,camera_.calculateMvp(ev.entityGlobalTransformation));

	animation::JointTransformation3D jointTransformations[data::Mesh::kMaxBones];
	rendering::animation::Animator::bindSkeleton(mesh,count,transforms,jointTransformations);
	rendering::Pipeline::Constant c("boneMatrices");
	services::rendering()->bind(c,(void*)jointTransformations);

	drawSubmesh(material,mesh);
}

}