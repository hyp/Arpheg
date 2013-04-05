#pragma once

#include "types.h"
//#include "events.h"
#include "../rendering/frustumCulling.h"
#include "../core/math.h"
#include "../core/memory.h"

namespace scene {
namespace rendering {

	typedef ::rendering::frustumCulling::Mask FrustumMask;

	core::Allocator* allocator();

	class  IEntity {
	public:
		virtual void draw(const Entity* self,const events::Draw& ev) = 0;
	};

	union EntityTransformation {
		struct {
			float rotation[4];
			float scaling[4];
			float translation[4];
		};
		float values[12];//A 3x3 + translation or quaternion + scaling + translation
	};

	struct CustomEntity {
		
		IEntity* entity;
	};

	STRUCT_PREALIGN(16) struct Entity {
		EntityTransformation transformation;
		uint32 lastRenderedFrame;
		uint32 cullId;
		uint32 flags_occCullId;
		//Mesh + material.
		data::SubMesh*  mesh;
		data::Material* material;
	} STRUCT_POSTALIGN(16);

	struct SkeletalAnimation;
	STRUCT_PREALIGN(16) struct AnimatedEntity {
		EntityTransformation transformation;
		uint32 lastRenderedFrame;
		uint32 cullId;
		uint32 flags_occCullId;
		uint32 animationId;
		data::SubMesh*  mesh;
		data::Material* material;
	} STRUCT_PREALIGN(16);

	STRUCT_PREALIGN(16) struct LightEntity {
		enum { Directional,Point,Spot, kCastsShadow = 0x100 };
		uint32 typeflags;
		uint32 cullId;
		::rendering::Light light;
	} STRUCT_PREALIGN(16);

	//The Entity Grid is a grid used to cull the whole scene.
	struct EntityGridCell {
        enum { kMaxBlocks = 16 };
        enum { kMaxBlockEntities = 1024 };
        
        //Sphere frustum culling.
        struct Block {
            vec4f*  positions;//Contain the spheres for frustum culling.
            EntityId* entities;
        };
        //AABB frustum/occlusion culling.
        struct BoxBlock {
            struct AABB { vec4f min; vec4f max; };
            AABB* positions;
            EntityId* entities;
        };
		template<typename T> struct BlockGroup {
			uint16 count[kMaxBlocks];
			T blocks[kMaxBlocks];
		};

        uint16 totalSphereCount_,totalBoxCount_,totalOccCount_;
        //World AABB - check self before culling the children.
        vec4f aabbMin, aabbMax;
		BlockGroup<Block>    sphereFrustumCullers;
		BlockGroup<BoxBlock> boxFrustumCullers;
		BlockGroup<BoxBlock> boxOcclusionCullers;

		EntityGridCell();
        CullId createSphere(const vec4f& position,EntityId entity);
		CullId createBox   (const vec4f& min,const vec4f& max,EntityId entity);
		CullId createOcclusionBox(const vec4f& min,const vec4f& max,EntityId entity);

	private:
	};

	struct EntityGrid;

	template<typename T>
	struct EntityPool;

	// This service provides a threadsafe way to create and manage renderable entities in the scene.
	// It is also responsible for culling and dispatching the rendering events to visible entities.
	class Service {
		enum { kDefaultBlocks = 16 };
		enum { kMaxBlockEntities = 1024 };
	public:
		EntityPool<Entity>*  entities_;
		EntityPool<AnimatedEntity>* animatedEntities_;
		EntityPool<CustomEntity>*   customEntities_;
		
		core::BufferAllocator* visibleEntities_;
		core::BufferAllocator* visibleAnimatedEntities_;
		EntityGrid* entityGrid_;
		core::BufferAllocator  skeletonAnimations_;
		
		Service(core::Allocator* allocator);
		~Service();
		void servicePreStep();

		EntityId create(data::Mesh* mesh,data::Material* material,const vec3f& position,const Quaternion& rotation,const vec3f& scale);
		void addAnimation(EntityId id,data::animation::Animation* animation,int count = 0,float t = 0.f);
		void removeAnimation(EntityId id,data::animation::Animation* animation);
		
		void update(EntityId id,const vec3f& position,bool updatePosition,const Quaternion& rotation,bool updateRotation,const vec3f& scale,bool updateScale);
		void remove(EntityId id);

		void markVisibleEntity(const EntityGridCell* cell,FrustumMask mask,EntityId id);
		void spawnFrustumCullingTasks(::rendering::frustumCulling::Frustum* frustums,size_t count);
		void spawnAnimationTasks();
		void render(events::Draw& ev);
		inline size_t renderedEntityCount() const;
	private:
		bool visibleEntitiesInited;
		size_t drawnEntities;
	};
	inline size_t Service::renderedEntityCount() const { return drawnEntities; }
} }