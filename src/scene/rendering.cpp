#include "../core/assert.h"
#include "../core/memory.h"
#include "../core/allocatorNew.h"
#include "../core/bufferArray.h"
#include "../core/thread/thread.h"
#include "../rendering/frustumCulling.h"
#include "../rendering/debug.h"
#include "../rendering/animation.h"
#include "../rendering/lighting/lighting.h"
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

template<typename T,typename Shape>
static inline void getShape(const EntityGridCell::BlockGroup<T>& group,uint32 id,Shape& result){
	auto block = id>>kBlockIdOffset;
	result = group.blocks[block].positions[id&kEntityIdMask];
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
	vec4f  getFrustumCullSphere(CullId id);

	EntityGrid();
};
CullId EntityGrid::createFrustumCullSphere(const vec4f& sphere,EntityId entity){
	vec4f s = sphere;
	s.w = -s.w; //NB: frustum culler expects sphere in format center, - radius
	return grid[0].createSphere(s,entity);
}
CullId EntityGrid::updateFrustumCullSphere(CullId id,const vec4f& sphere){
	//grid[0].update(id&kEntityBlockIdMask,sphere);
	return id;
}
void   EntityGrid::removeFrustumCullSphere(CullId id){
	//grid[0].remove(id.id&kEntityBlockIdMask);
}
vec4f  EntityGrid::getFrustumCullSphere(CullId id) {
	vec4f sphere;
	getShape(grid[0].sphereFrustumCullers,id.id,sphere);
	return sphere;
}
EntityGrid::EntityGrid(){
	//TODO
}

//Entity manager
enum {
	kEntityPtrMask = 0xFFFFFF,
	kEntityTypeOffset = 24,

	kNormalEntity = 0,
	kAnimatedEntity = 1,
	kCustomEntity   = 2,
	kLightEntity    = 3,

	flagHasFrustumCuller = 1,
};

struct VisibleEntity {
	FrustumMask mask;
	Entity* entity;
	uint32 offset,size;
};
struct VisibleAnimatedEntity {
	FrustumMask mask;
	AnimatedEntity* entity;
	uint32 offset,size;
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

struct VisibleLightEntity {
	FrustumMask mask;
	LightEntity* entity;
};

Service::Service(core::Allocator* alloc) :
	skeletonAnimations_(sizeof(SkeletalAnimation)*1024,allocator(),core::BufferAllocator::GrowOnOverflow),
	entityConstantStorage(1024*8,allocator(),core::BufferAllocator::GrowOnOverflow)
{
	//Entity pools.
	entities_ = ALLOCATOR_NEW(alloc,EntityPool<Entity>) (1024*32,allocator());
	animatedEntities_ = ALLOCATOR_NEW(alloc,EntityPool<AnimatedEntity>) (1024*2,allocator());
	customEntities_ = ALLOCATOR_NEW(alloc,EntityPool<CustomEntity>) (1024*2,allocator());
	lights_ = ALLOCATOR_NEW(alloc,EntityPool<LightEntity>) (1024*2,allocator());

	visibleEntities_ = (core::BufferAllocator*)alloc->allocate(sizeof(core::BufferAllocator)*services::tasking()->threadCount(),alignof(core::BufferAllocator));
	visibleAnimatedEntities_ = (core::BufferAllocator*)alloc->allocate(sizeof(core::BufferAllocator)*services::tasking()->threadCount(),alignof(core::BufferAllocator));
	visibleLights_ = (core::BufferAllocator*)alloc->allocate(sizeof(core::BufferAllocator)*services::tasking()->threadCount(),alignof(core::BufferAllocator));
	
	visibleEntitiesInited = false;
	entityGrid_ = ALLOCATOR_NEW(alloc,EntityGrid);
	lighting_ = ALLOCATOR_NEW(alloc,::rendering::lighting::Service);
}
Service::~Service() {
	entities_->~EntityPool();
	animatedEntities_->~EntityPool();
	customEntities_->~EntityPool();
	lights_->~EntityPool();

	if(visibleEntitiesInited){
		for(size_t i = 0;i< services::tasking()->threadCount();++i){
			visibleEntities_[i].~BufferAllocator();
			visibleAnimatedEntities_[i].~BufferAllocator();
			visibleLights_[i].~BufferAllocator();
		}
	}

	entityGrid_->~EntityGrid();
	lighting_->~Service();
}
void Service::servicePreStep() {
	activeCameras = 0;
	if(!visibleEntitiesInited){
		for(size_t i = 0;i< services::tasking()->threadCount();++i){
			new(visibleEntities_ + i) core::BufferAllocator(sizeof(VisibleEntity)*1024,allocator(),core::BufferAllocator::GrowOnOverflow);
			new(visibleAnimatedEntities_ + i) core::BufferAllocator(sizeof(VisibleAnimatedEntity)*1024,allocator(),core::BufferAllocator::GrowOnOverflow);
			new(visibleLights_ + i) core::BufferAllocator(sizeof(VisibleLightEntity)*1024,allocator(),core::BufferAllocator::GrowOnOverflow);
		}
		visibleEntitiesInited = true;
	} else {
		for(size_t i = 0;i< services::tasking()->threadCount();++i){
			visibleEntities_[i].reset();
			visibleAnimatedEntities_[i].reset();
			visibleLights_[i].reset();
		}
	}
	drawnEntities = 0;
	lighting_->servicePreStep();
	entityConstantStorage.reset();
}

static inline void storeTransform(EntityTransformation& transformation,const vec3f& position,const Quaternion& rotation,const vec3f& scale){
	rotation.v().store(transformation.rotation);
	vec4f(position.x,position.y,position.z,0.f).store(transformation.translation);
	vec4f(scale.x,scale.y,scale.z,0.f).store(transformation.scaling);
}
static inline SkeletalAnimation* getSkeletalAnimation(core::BufferAllocator& buffer,AnimatedEntity* entity){
	return (SkeletalAnimation*)buffer.toPointer(entity->animationId*sizeof(SkeletalAnimation));
}
static inline uint32 createFrustumCuller(EntityId id,vec3f position,vec3f scale,data::Mesh* mesh,EntityGrid* grid){
	vec4f sp = vec4f(position) + vec4f(mesh->frustumShapeOffset) * vec4f(scale);
	vec4f ss = vec4f(mesh->frustumShapeSize) * vec4f(scale);
	float r = std::max(ss.x,std::max(ss.y,ss.z));
	return grid->createFrustumCullSphere(vec4f(sp.x,sp.y,sp.z,r),id).id;
}
static inline uint32 createEntityId(uint32 ptr,uint32 type){
	return ptr | (type << kEntityTypeOffset);
}
static inline bool isEntityOfType(EntityId id,uint32 type){
	return (id.id & (type << kEntityTypeOffset))!=0;
}

EntityId Service::create(data::Mesh* mesh,data::Material* material,const vec3f& position,const Quaternion& rotation,const vec3f& scale) {
	if(mesh->hasSkeleton()){
		auto dest = animatedEntities_->allocate();
		EntityId result = { createEntityId(animatedEntities_->toId(dest),kAnimatedEntity) };
		dest->cullId = createFrustumCuller(result,position,scale,mesh,entityGrid_);
		dest->flags_occCullId = 0;
		auto anim = ALLOCATOR_NEW(&skeletonAnimations_,SkeletalAnimation) (result,mesh);
		dest->animationId = skeletonAnimations_.toOffset(anim)/sizeof(SkeletalAnimation);
		storeTransform(dest->transformation,position,rotation,scale);
		dest->mesh = mesh->submesh(0);
		dest->material = material;
		return result;
	} else {
		auto dest = entities_->allocate();
		EntityId result= { createEntityId(entities_->toId(dest),kNormalEntity) };
		dest->cullId = createFrustumCuller(result,position,scale,mesh,entityGrid_);
		dest->flags_occCullId = 0;
		storeTransform(dest->transformation,position,rotation,scale);
		dest->mesh = mesh->submesh(0);dest->material = material;
		return result;
	}//return create(mesh->submesh(0),material,position,rotation,scale);
}
void Service::addAnimation(EntityId id,data::animation::Animation* animation,int count,float t) {
	assert(isEntityOfType(id,kAnimatedEntity));

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

EntityId Service::createLight(const ::rendering::Light& light,uint32 flags){
	auto dest = lights_->allocate();
	EntityId result= { createEntityId(lights_->toId(dest),kLightEntity) };
	if(light.isPoint()){
		dest->typeflags = flags | LightEntity::Point;
		dest->cullId = entityGrid_->createFrustumCullSphere(light.sphere(),result).id;
	} else {
		dest->typeflags = flags | LightEntity::Directional;
		dest->cullId = 0;
	}
	dest->light = light;
	return result;
}

void Service::remove(EntityId id){
	//TODO
	entities_->free(entities_->toPtr(id.id&kEntityPtrMask));
}

void Service::setActiveCameras(::rendering::Camera* cameras,size_t count) {
	assert(count < kMaxActiveCameras);
	activeCameras = (uint32)std::min(count,size_t(kMaxActiveCameras));
	static int cc = 0;
	for(uint32 i = 0;i < activeCameras;++i){
		this->cameras[i] = cameras[i];
	}
}

void Service::markVisibleEntity(const EntityGridCell* cell,FrustumMask mask,EntityId id) {
	auto threadId = services::tasking()->threadId();
	auto type = id.id >> kEntityTypeOffset;
	auto eid  = id.id & kEntityPtrMask;

	switch(type){
	case kNormalEntity: {
		auto entity = (VisibleEntity*)visibleEntities_[threadId].allocate(sizeof(VisibleEntity));
		entity->mask = mask;
		entity->entity = entities_->toPtr(eid);
		} break;
	case kAnimatedEntity: {
		auto entity = (VisibleAnimatedEntity*)visibleAnimatedEntities_[threadId].allocate(sizeof(VisibleAnimatedEntity));
		entity->mask = mask;
		entity->entity = animatedEntities_->toPtr(eid);
		} break;
	case kCustomEntity: {
		} break;
	case kLightEntity: {
		//Lights are filtered separately.
		auto entity = (VisibleLightEntity*)visibleLights_[threadId].allocate(sizeof(VisibleLightEntity));
		entity->mask = mask;
		entity->entity = lights_->toPtr(eid);
		} break;
	}
}
void Service::spawnLightProcessingTasks() {
	assert(activeCameras);

	using namespace ::core::bufferArray;
	using namespace ::rendering::lighting;

	uint32 viewCount = 1;

	::rendering::Viewport viewports[kMaxActiveCameras];
	viewports[0].position = vec2i(0,0);
	viewports[0].size = services::rendering()->context()->frameBufferSize();

	lighting_->setActiveViewports(viewports,viewCount);

	//Merge visible lights from the thread buffers.
	size_t offset = 0;
	TileGrid::Tiler tiler(cameras[0],viewports[0]);
	for(size_t i = 0;i< services::tasking()->threadCount();++i){
		auto count = length<VisibleLightEntity>(visibleLights_[i]);
		if(offset + count >lighting_->maxActiveLights()) break;
		
		uint32 view = 0;
		uint32 viewMask = 1;
		
		for(VisibleLightEntity* ent = begin<VisibleLightEntity>(visibleLights_[i]),*entend = end<VisibleLightEntity>(visibleLights_[i]);ent<entend;++ent){
			lighting_->addActiveLight(ent->entity->light);
			if(ent->mask & 1) tiler.tile(ent->entity->light.sphere(),TileGrid::LightIndex(offset));
			++offset;
		}
		
		lighting_->tileGrid(view)->performLightAssignment(tiler);
		//The other views.
		for(;;){
			++view;viewMask<<=1;
			if(view >= viewCount) break;
			for(VisibleLightEntity* ent = begin<VisibleLightEntity>(visibleLights_[i]),*entend = end<VisibleLightEntity>(visibleLights_[i]);ent<entend;++ent){
				if(ent->mask & viewMask) tiler.tile(ent->entity->light.sphere(),TileGrid::LightIndex(offset));
			}
		}
	}

	lighting_->tileGrid(0)->performLightAssignment(tiler);
}
void Service::spawnFrustumCullingTasks() {
	assert(activeCameras);

	using namespace ::rendering::frustumCulling;
	Culler culler(this);
	for(uint32 i = 0; i< activeCameras;++i){
		culler.addFrustum(Frustum(cameras[i]));
	}

	auto cell = entityGrid_->grid;
	if(cell->totalSphereCount_ > 0) cullSpheres(&culler,cell);
	if(cell->totalBoxCount_ > 0)    cullAABBs(&culler,cell);
}
void Service::transferLightData() {
	lighting_->transferData();
}

static inline void getMatrices(const EntityTransformation& transformation,mat44f& world){
	Quaternion rotation(vec4f::load(transformation.rotation));
	vec3f translation(transformation.translation[0],transformation.translation[1],transformation.translation[2]);
	vec3f scaling(transformation.scaling[0],transformation.scaling[1],transformation.scaling[2]);
	world = mat44f::translateRotateScale(translation,rotation,scaling);
}
static void drawFrustumShape(EntityGrid* grid,uint32 id){
	CullId cullId = { id };
	auto sphere = grid->getFrustumCullSphere(cullId);
	services::debugRendering()->sphere(mat44f::identity(),sphere.xyz(),fabs(sphere.w),vec4f(0.f,0.f,1.f,1.f));
}
void Service::render(events::Draw& ev){
	using namespace core::bufferArray;
	auto frustumMask = 1<<(0);
	assert(services::tasking()->isRenderingThread());

	struct VV {
		mat44f view;
		float oneOverTileX;int tileWidths;int maxTiles;
	};
	VV view = { cameras[0].view,1.0f/32.0f,40,lighting_->tileGrid(0)->maxLightsPerTile()};
	viewConstants.update(core::Bytes(&view,sizeof(view)));

	for(size_t i = 0;i< services::tasking()->threadCount();++i){
		for(VisibleEntity* ent = begin<VisibleEntity>(visibleEntities_[i]),*entend = end<VisibleEntity>(visibleEntities_[i]);ent<entend;++ent){
			if(ent->mask & frustumMask){
				auto entity = ent->entity;
				mat44f world; 
				getMatrices(entity->transformation,world);
				world = cameras[0].calculateMvp(world);
				float* dest = (float*)entityConstantStorage.allocate(sizeof(mat44f) + sizeof(vec4f)*3,0);
				ent->offset = entityConstantStorage.toOffset(dest);
				ent->size = sizeof(mat44f)+sizeof(vec4f)*3;
				((mat44f*)dest)[0] = world;
				dest+=16;
				vec4f::load(entity->transformation.scaling).store(dest);dest+=4;
				vec4f::load(entity->transformation.rotation).store(dest);dest+=4;
				vec4f::load(entity->transformation.translation).store(dest);dest+=4;
				entityConstantStorage.allocate(services::rendering()->context()->constantBufferOffsetAlignment() - (sizeof(mat44f) + sizeof(vec4f)*3),0);
			}
		}
		for(VisibleAnimatedEntity* ent = begin<VisibleAnimatedEntity>(visibleAnimatedEntities_[i]),*entend = end<VisibleAnimatedEntity>(visibleAnimatedEntities_[i]);
			ent<entend;++ent){
				auto entity = ent->entity;
				mat44f world; 
				getMatrices(entity->transformation,world);
				world = cameras[0].calculateMvp(world);
				float* dest = (float*)entityConstantStorage.allocate(sizeof(mat44f) + sizeof(vec4f)*3,0);
				ent->offset = entityConstantStorage.toOffset(dest);
				ent->size = sizeof(mat44f)+sizeof(vec4f)*3;
				((mat44f*)dest)[0] = world;
				dest+=16;
				vec4f::load(entity->transformation.scaling).store(dest);dest+=4;
				vec4f::load(entity->transformation.rotation).store(dest);dest+=4;
				vec4f::load(entity->transformation.translation).store(dest);dest+=4;
				entityConstantStorage.allocate(services::rendering()->context()->constantBufferOffsetAlignment() - (sizeof(mat44f) + sizeof(vec4f)*3),0);
		}
	}
	entityConstants.update(core::Bytes(entityConstantStorage.bufferBase(),entityConstantStorage.size()));

	for(size_t i = 0;i< services::tasking()->threadCount();++i){
		for(VisibleEntity* ent = begin<VisibleEntity>(visibleEntities_[i]),*entend = end<VisibleEntity>(visibleEntities_[i]);ent<entend;++ent){
			if(ent->mask & frustumMask){
				auto entity = ent->entity;
	
				ev.meshRenderer->draw(ev,viewConstants.buffer,entityConstants.buffer,ent->offset,ent->size,entity->mesh,entity->material);
				drawnEntities++;

				drawFrustumShape(entityGrid_,entity->cullId);
			}
		}
		for(VisibleAnimatedEntity* ent = begin<VisibleAnimatedEntity>(visibleAnimatedEntities_[i]),*entend = end<VisibleAnimatedEntity>(visibleAnimatedEntities_[i]);
			ent<entend;++ent){
			if(ent->mask & frustumMask){
				auto entity = ent->entity;
				auto anim = getSkeletalAnimation(skeletonAnimations_,entity);
				ev.meshRenderer->draw(ev,viewConstants.buffer,entityConstants.buffer,ent->offset,ent->size,entity->mesh,entity->material,anim->transformations,anim->nodeCount);
				drawnEntities++;
				
				drawFrustumShape(entityGrid_,entity->cullId);
			}
		}
	}
}

} } 