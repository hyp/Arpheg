#pragma once

#include "../data/types.h"

namespace scene {

// Culling,rendering,lighting, etc.
namespace rendering {
	class  IEntity;
	struct Entity;
	struct EntityGridCell;
	class  Service;

	//Strong typedefs
	struct CullId   {
		uint32 id;
	};
	struct EntityId {
		uint32 id;
	};
}

// Physics and collisions.
namespace physics {
	class Service;
}

// Positional sound effects.
namespace sound {
	class Service; 
}

namespace events {
	class IMeshRenderer;
	struct Draw;
}

namespace events {
	struct Draw {
		mat44f entityGlobalTransformation;
		IMeshRenderer* meshRenderer;
	};
	class IMeshRenderer {
	public:
		virtual void draw(const Draw& ev,::rendering::Buffer view,::rendering::Buffer buffer, uint32 offset,uint32 size,const data::SubMesh* mesh,const data::Material* material) = 0;
		virtual void draw(const Draw& ev,::rendering::Buffer view,::rendering::Buffer buffer, uint32 offset,uint32 size,const data::SubMesh* mesh,const data::Material* material,const data::Transformation3D* transforms,uint32 count) = 0;
	};
}

}