#include "../core/assert.h"
#include "../core/memory.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../core/thread/thread.h"
#include "../rendering/frustumCulling.h"
#include "../rendering/debug.h"
#include "../rendering/animation.h"
#include "../services.h"
#include "rendering.h"
#include "entityPool.h"

namespace scene {
namespace rendering {

core::Allocator* allocator() { return core::memory::globalAllocator(); }

// Cull primitives:
// Maximum 1024 per block, maximum 32 blocks per grid cell, maximum 32k grid cells
// -> Choose cell size for upto 65K entities per cell at once.

enum {
	kEntityIdMask = 1024 - 1,//2^10 - 1
	kBlockIdOffset = 10,
	kBlockIdMask = 32 - 1, //2^5 - 1
	kBoxFlag = 0x8000, //Indicates that the cull shape is an AABB.
	kEntityBlockIdMask = 0xFFFF,//2^16 - 1
	kGridIdOffset = 16,
};

template<typename T>
static inline void initGroup(EntityGridCell::BlockGroup<T>& group){
	for(uint32 i = 0;i<EntityGridCell::kMaxBlocks;++i){
		group.count[i] = 0;
		group.blocks[i].positions = nullptr;
		group.blocks[i].entities = nullptr;
	}
}

template<typename T,typename Shape>
static inline void createGroupBlock(core::Allocator* allocator,EntityGridCell::BlockGroup<T>& group,uint32 i){
	assert(i < EntityGridCell::kMaxBlocks);
	assert(group.blocks[i].positions == nullptr);

	group.blocks[i].positions = (Shape*)allocator->allocate(EntityGridCell::kMaxBlockEntities*sizeof(Shape),alignof(Shape));
	group.blocks[i].entities = (EntityId*)allocator->allocate(EntityGridCell::kMaxBlockEntities*sizeof(EntityId),alignof(EntityId));
}

template<typename T,typename Shape>
static inline uint32 allocateGroupBlock(EntityGridCell::BlockGroup<T>& group,const Shape& obj,EntityId entity){
	for(uint32 i = 0;i<EntityGridCell::kMaxBlocks;++i){
		if(group.count[i] < EntityGridCell::kMaxBlockEntities){
			auto id = uint32(group.count[i]);
			if(id == 0 && !group.blocks[i].positions) createGroupBlock<T,Shape>(allocator(),group,i);
			group.blocks[i].positions[id] = obj;
			group.blocks[i]. entities[id] = entity;
			group.count [i]++; 
			return (i<<kBlockIdOffset)|id;
		}
	}
	assert(false && "Not enough space in scene's rendering grid block for a frustum culler!");
	return 0;
}

EntityGridCell::EntityGridCell() {
	initGroup(sphereFrustumCullers);
	initGroup(boxFrustumCullers);
	initGroup(boxOcclusionCullers);
	totalSphereCount_ = 0; totalBoxCount_ = 0; totalOccCount_ = 0;
}
CullId EntityGridCell::createSphere(const vec4f& position,EntityId entity){
	CullId result;
	result.id = allocateGroupBlock(sphereFrustumCullers,position,entity);
	totalSphereCount_++;
	return result;
}
CullId EntityGridCell::createBox   (const vec4f& min,const vec4f& max,EntityId entity) {
	CullId result;
	BoxBlock::AABB aabb = { min,max };
	result.id = allocateGroupBlock(boxFrustumCullers,aabb,entity)|kBoxFlag;
	totalBoxCount_++;
	return result;
}
CullId EntityGridCell::createOcclusionBox(const vec4f& min,const vec4f& max,EntityId entity) {
	CullId result;
	BoxBlock::AABB aabb = { min,max };
	result.id = allocateGroupBlock(boxOcclusionCullers,aabb,entity);
	totalOccCount_++;
	return result;
}

struct EntityGrid {
	EntityGridCell grid[10];

	CullId createFrustumCullSphere(const vec4f& sphere,EntityId entity);
	CullId updateFrustumCullSphere(CullId id,const vec4f& sphere);
	void   removeFrustumCullSphere(CullId id);

	EntityGrid();
};
CullId EntityGrid::createFrustumCullSphere(const vec4f& sphere,EntityId entity){
	return grid[0].createSphere(sphere,entity);
}
CullId EntityGrid::updateFrustumCullSphere(CullId id,const vec4f& sphere){
	//grid[0].update(id&kEntityBlockIdMask,sphere);
	return id;
}
void   EntityGrid::removeFrustumCullSphere(CullId id){
	//grid[0].remove(id.id&kEntityBlockIdMask);
}
EntityGrid::EntityGrid(){
	//TODO
}

//Entity manager
enum {
	kEntityPtrMask = 0xFFFFFF,
	kEntityTypeOffset = 24,

	kNormalEntity = 0,
	kAnimatedEntity = 0x1000000,
	kCustomEntity = 0x2000000,

	flagHasFrustumCuller = 1,
};

struct VisibleEntity {
	FrustumMask mask;
	Entity* entity;
};
struct VisibleAnimatedEntity {
	FrustumMask mask;
	AnimatedEntity* entity;
};
struct VisibleCustomEntity {
	FrustumMask mask;
	CustomEntity* entity;
};
struct SkeletalAnimation {
	enum { kMaxAnimations = 3 };

	EntityId entity;
	uint32 nodeCount;
	data::Transformation3D* transformations;
	data::Mesh* mesh;
	struct Anim {
		float time;
		data::animation::Animation* animation;
	};
	Anim anim[kMaxAnimations];

	SkeletalAnimation(EntityId id,data::Mesh* m) {
		entity = id;
		mesh = m;
		nodeCount= m->skeletonNodeCount();
		transformations = (data::Transformation3D*)allocator()->allocate(sizeof(data::Transformation3D)*mesh->skeletonNodeCount(),alignof(vec4f));
		for(uint32 i = 0;i<kMaxAnimations;++i){
			anim[i].time = 0.f;
			anim[i].animation = nullptr;
		}
	}
	~SkeletalAnimation() {
		allocator()->deallocate(transformations);
	}
	void animate(float dt) {
		if(anim[0].animation){
			anim[0].time += dt;
			::rendering::animation::Animator::animate(mesh,anim[0].animation,anim[0].time,transformations);
		}
		else ::rendering::animation::Animator::setDefaultSkeleton(mesh,transformations);
	}
	void add(data::animation::Animation* animation,int count,float t){
		for(uint32 i = 0;i<kMaxAnimations;++i){
			if(!anim[i].animation){
				anim[i].animation = animation;
				anim[i].time = t;
				return;
			}
		}
	}
};

Service::Service(core::Allocator* alloc) :
	skeletonAnimations_(sizeof(SkeletalAnimation)*1024,allocator(),core::BufferAllocator::GrowOnOverflow) 
{
	//Entity pools.
	entities_ = ALLOCATOR_NEW(alloc,EntityPool<Entity>) (1024*32,allocator());
	animatedEntities_ = ALLOCATOR_NEW(alloc,EntityPool<AnimatedEntity>) (1024*2,allocator());
	customEntities_ = ALLOCATOR_NEW(alloc,EntityPool<CustomEntity>) (1024*2,allocator());

	visibleEntities_ = (core::BufferAllocator*)alloc->allocate(sizeof(core::BufferAllocator)*services::tasking()->threadCount(),alignof(core::BufferAllocator));
	visibleAnimatedEntities_ = (core::BufferAllocator*)alloc->allocate(sizeof(core::BufferAllocator)*services::tasking()->threadCount(),alignof(core::BufferAllocator));
	visibleEntitiesInited = false;
	entityGrid_ = ALLOCATOR_NEW(alloc,EntityGrid);
}
Service::~Service() {
	entities_->~EntityPool();
	animatedEntities_->~EntityPool();
	customEntities_->~EntityPool();

	if(visibleEntitiesInited){
		for(size_t i = 0;i< services::tasking()->threadCount();++i){
			visibleEntities_[i].~BufferAllocator();
			visibleAnimatedEntities_[i].~BufferAllocator();
		}
	}
}
void Service::servicePreStep() {
	if(!visibleEntitiesInited){
		for(size_t i = 0;i< services::tasking()->threadCount();++i){
			new(visibleEntities_ + i) core::BufferAllocator(sizeof(VisibleEntity)*1024,allocator(),core::BufferAllocator::GrowOnOverflow);
			new(visibleAnimatedEntities_ + i) core::BufferAllocator(sizeof(VisibleAnimatedEntity)*1024,allocator(),core::BufferAllocator::GrowOnOverflow);
		}
		visibleEntitiesInited = true;
	} else {
		for(size_t i = 0;i< services::tasking()->threadCount();++i){
			visibleEntities_[i].reset();
			visibleAnimatedEntities_[i].reset();
		}
	}
	drawnEntities = 0;
}
static inline void storeTransform(EntityTransformation& transformation,const vec3f& position,const Quaternion& rotation,const vec3f& scale){
	rotation.v().store(transformation.rotation);
	vec4f(position.x,position.y,position.z,0.f).store(transformation.translation);
	vec4f(scale.x,scale.y,scale.z,0.f).store(transformation.scaling);
}
EntityId Service::create(data::SubMesh* mesh,data::Material* material,const vec3f& position,const Quaternion& rotation,const vec3f& scale){
	auto dest = entities_->allocate();
	EntityId result= { entities_->toId(dest) };
	dest->cullId = entityGrid_->createFrustumCullSphere(vec4f(position.x,position.y,position.z,std::max(scale.x,std::max(scale.z,scale.y))),result).id;
	dest->flags_occCullId = 0;
	storeTransform(dest->transformation,position,rotation,scale);
	dest->mesh = mesh;dest->material = material;
	return result;
}
static inline SkeletalAnimation* getSkeletalAnimation(core::BufferAllocator& buffer,AnimatedEntity* entity){
	return (SkeletalAnimation*)buffer.toPointer(entity->animationId*sizeof(SkeletalAnimation));
}
EntityId Service::create(data::Mesh* mesh,data::Material* material,const vec3f& position,const Quaternion& rotation,const vec3f& scale) {
	if(mesh->hasSkeleton()){
		auto dest = animatedEntities_->allocate();
		EntityId result = { animatedEntities_->toId(dest) | kAnimatedEntity };
		dest->cullId = entityGrid_->createFrustumCullSphere(vec4f(position.x,position.y,position.z,std::max(scale.x,std::max(scale.z,scale.y))),result).id;
		dest->flags_occCullId = 0;
		auto anim = ALLOCATOR_NEW(&skeletonAnimations_,SkeletalAnimation) (result,mesh);
		dest->animationId = skeletonAnimations_.toOffset(anim)/sizeof(SkeletalAnimation);
		storeTransform(dest->transformation,position,rotation,scale);
		dest->mesh = mesh->submesh(0);
		dest->material = material;
		return result;
	} else return create(mesh->submesh(0),material,position,rotation,scale);
}
void Service::addAnimation(EntityId id,data::animation::Animation* animation,int count,float t) {
	assert((id.id & kAnimatedEntity)!=0);
	auto anim = getSkeletalAnimation(skeletonAnimations_,animatedEntities_->toPtr(id.id&kEntityPtrMask));
	anim->add(animation,count,t);
}
void Service::removeAnimation(EntityId id,data::animation::Animation* animation) {
	assert(id.id & kAnimatedEntity);
	auto anim = getSkeletalAnimation(skeletonAnimations_,animatedEntities_->toPtr(id.id&kEntityPtrMask));
}
void Service::spawnAnimationTasks() {
	using namespace core::bufferArray;
	auto dt = services::timing()->dt();
	for(auto i = begin<SkeletalAnimation>(skeletonAnimations_);i < end<SkeletalAnimation>(skeletonAnimations_);++i){
		i->animate(dt);
	}
}
void Service::update(EntityId id,const vec3f& position,bool updatePosition,const Quaternion& rotation,bool updateRotation,const vec3f& scale,bool updateScale) {
	//TODO
}

void Service::remove(EntityId id){
	//TODO
	entities_->free(entities_->toPtr(id.id&kEntityPtrMask));
}


void Service::markVisibleEntity(const EntityGridCell* cell,FrustumMask mask,EntityId id) {
	auto type = id.id & (~kEntityPtrMask);
	if(type == 0){
		auto entity = (VisibleEntity*)visibleEntities_[services::tasking()->threadId()].allocate(sizeof(VisibleEntity));
		entity->mask = mask;
		entity->entity = entities_->toPtr(id.id&kEntityPtrMask);
	} else if(type == kAnimatedEntity){
		auto entity = (VisibleAnimatedEntity*)visibleAnimatedEntities_[services::tasking()->threadId()].allocate(sizeof(VisibleAnimatedEntity));
		entity->mask = mask;
		entity->entity = animatedEntities_->toPtr(id.id&kEntityPtrMask);
	} else if(type == kCustomEntity){

	}
}
void Service::spawnFrustumCullingTasks(::rendering::frustumCulling::Frustum* frustums,size_t count) {
	assert(frustums && count);

	using namespace ::rendering::frustumCulling;
	Culler culler(this);
	for(size_t i = 0;i<count;++i) culler.addFrustum(frustums[i]);
	auto cell = entityGrid_->grid;
	if(cell->totalSphereCount_ > 0) cullSpheres(&culler,cell);
	if(cell->totalBoxCount_ > 0)    cullAABBs(&culler,cell);
}
static inline void getMatrices(EntityTransformation& transformation,mat44f& world){
	Quaternion rotation(vec4f::load(transformation.rotation));
	vec3f translation(transformation.translation[0],transformation.translation[1],transformation.translation[2]);
	vec3f scaling(transformation.scaling[0],transformation.scaling[1],transformation.scaling[2]);
	world = mat44f::translateRotateScale(translation,rotation,scaling);
}
void Service::render(events::Draw& ev){
	using namespace core::bufferArray;
	auto frustumMask = 1<<(0);

	for(size_t i = 0;i< services::tasking()->threadCount();++i){
		for(VisibleEntity* ent = begin<VisibleEntity>(visibleEntities_[i]),*entend = end<VisibleEntity>(visibleEntities_[i]);ent<entend;++ent){
			if(ent->mask & frustumMask){
				auto entity = ent->entity;
				getMatrices(entity->transformation,ev.entityGlobalTransformation);
				ev.meshRenderer->draw(ev,entity->mesh,entity->material);
				drawnEntities++;

				services::debugRendering()->sphere(ev.entityGlobalTransformation,vec3f(0,0,0),1.0f,vec4f(0,0,1,1));
			}
		}
		for(VisibleAnimatedEntity* ent = begin<VisibleAnimatedEntity>(visibleAnimatedEntities_[i]),*entend = end<VisibleAnimatedEntity>(visibleAnimatedEntities_[i]);
			ent<entend;++ent){
			if(ent->mask & frustumMask){
				auto entity = ent->entity;
				getMatrices(entity->transformation,ev.entityGlobalTransformation);
				auto anim = getSkeletalAnimation(skeletonAnimations_,entity);
				ev.meshRenderer->draw(ev,entity->mesh,entity->material,anim->transformations,anim->nodeCount);
				drawnEntities++;
				services::debugRendering()->sphere(ev.entityGlobalTransformation,vec3f(0,0,0),1.0f,vec4f(0,0,1,1));
			}
		}
	}
}

} } 