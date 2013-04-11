#include "../core/assert.h"
#include "../services.h"
#include "lighting/lighting.cpp"
#include "../scene/rendering.h"
#include "animation.h"
#include "3d.h"

namespace rendering {


	
static void drawSubmesh(Pipeline::Constant& tc,const data::Material* material,const data::SubMesh* submesh) {
	
	
	auto renderer = services::rendering();
	if(material){
		uint32 slots[::data::Material::kMaxTextures] = {0};
		for(uint32 i = 0;i<material->textureCount();++i)
			renderer->bind(material->textures()[i],i);
		renderer->bind(tc,slots);
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
rendering::Pipeline::Constant bmC("boneMatrices");
rendering::Pipeline::Constant tCa("texture");
rendering::Pipeline::Constant tCb("texture");
rendering::Pipeline::Constant tTTa("tiledLightingTextures");
rendering::Pipeline::Constant tTTb("tiledLightingTextures");
rendering::Pipeline::Constant xTTa("lights");
rendering::Pipeline::Constant xTTb("lights");
void DirectMeshRenderer::bind(data::Pipeline* pipeline,data::Pipeline* anim) {
	staticMesh_ = pipeline->pipeline();mvp = &pipeline->modelViewProjectionConstant();
	animatedMesh_ = anim->pipeline();amvp = &anim->modelViewProjectionConstant();

	int slots[] = {1,2,3};
	services::rendering()->bindConstantSlot(staticMesh_,rendering::Pipeline::Constant("objects"),0);
	services::rendering()->bindConstantSlot(staticMesh_,rendering::Pipeline::Constant("view"),1);
	services::rendering()->bindConstantSlot(staticMesh_,rendering::Pipeline::Constant("lights"),2);
	services::rendering()->bind(staticMesh_);
	services::rendering()->bind(tTTa,slots);
	//services::rendering()->bind(xTTa,slots+2);

	services::rendering()->bindConstantSlot(animatedMesh_,rendering::Pipeline::Constant("objects"),0);
	services::rendering()->bindConstantSlot(animatedMesh_,rendering::Pipeline::Constant("view"),1);
	services::rendering()->bindConstantSlot(animatedMesh_,rendering::Pipeline::Constant("lights"),2);
	services::rendering()->bind(animatedMesh_);
	services::rendering()->bind(tTTb,slots);
	//services::rendering()->bind(xTTb,slots+2);
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
void DirectMeshRenderer::draw(const scene::events::Draw& ev,::rendering::Buffer view,::rendering::Buffer buffer,uint32 offset,uint32 size,const data::SubMesh* mesh,const data::Material* material) {
	state_++;
	auto t0 = services::sceneRendering()->lighting_->tileGrid(0)->tileBufferTexture();
	auto t1 = services::sceneRendering()->lighting_->tileGrid(0)->indexBufferTexture();
	auto lb = services::sceneRendering()->lighting_->lightBuffer();
	
	if(state_ <= 1){
		services::rendering()->bind(t0,1);
		services::rendering()->bind(t1,2);
		//services::rendering()->bind(services::sceneRendering()->lighting_->tlights_.textureView,3);
		services::rendering()->bindConstantBuffer(lb,2);
		
	}

	services::rendering()->bind(staticMesh_);
	//services::rendering()->bind(Pipeline::Constant("lights"),services::sceneRendering()->lighting_->lights());
	services::rendering()->bindConstantBuffer(buffer,0,offset,size);
	services::rendering()->bindConstantBuffer(view,1);
	drawSubmesh(tCa,material,mesh);
}
void DirectMeshRenderer::draw(const scene::events::Draw& ev,::rendering::Buffer view,::rendering::Buffer buffer,uint32 offset,uint32 size,const data::SubMesh* mesh,const data::Material* material,const data::Transformation3D* transforms,uint32 count) {
	state_++;
	auto t0 = services::sceneRendering()->lighting_->tileGrid(0)->tileBufferTexture();
	auto t1 = services::sceneRendering()->lighting_->tileGrid(0)->indexBufferTexture();
	auto lb = services::sceneRendering()->lighting_->lightBuffer();
	if(state_ <= 1){
		services::rendering()->bind(t0,1);
		services::rendering()->bind(t1,2);
	//	services::rendering()->bind(services::sceneRendering()->lighting_->tlights_.textureView,3);
		services::rendering()->bindConstantBuffer(lb,2);
	}
	
	if(services::rendering()->currentPipeline != animatedMesh_.id){
		services::rendering()->bind(animatedMesh_);
		//services::rendering()->bind(Pipeline::Constant("lights"),services::sceneRendering()->lighting_->lights());
	}

	animation::JointTransformation3D jointTransformations[data::Mesh::kMaxBones];
	rendering::animation::Animator::bindSkeleton(mesh,count,transforms,jointTransformations);
	services::rendering()->bind(bmC,(void*)jointTransformations);
	services::rendering()->bindConstantBuffer(buffer,0,offset,size);
	services::rendering()->bindConstantBuffer(view,1);

	drawSubmesh(tCb,material,mesh);
}

}