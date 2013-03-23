#pragma once

#include "types.h"
#include "../core/memory.h"

namespace data {

	namespace internal_ {
		class Service;
	}

	class Service {
	public:
		Service(core::Allocator* allocator);
		~Service();
		
		//Provides direct loading of resources from the filesystem
		rendering::Texture2D loadTexture(const char* name);
		
		//Bundle based data management
		void loadBundle(const char* filename,ID id);
		void switchBundle(ID id);
		
		//raw rendering resources without extra cruff
		rendering::Texture2D      texture2D(ID id);
		rendering::Texture2DArray texture2DArray(ID id);
		rendering::Sampler sampler(ID id);

		Pipeline* pipeline(ID id);
		SubMesh*  submesh(ID id);
		Mesh* mesh(ID id);
		animation::Animation* animation(ID id);
		Font* font(ID id);
		Sprite* sprite(ID id);

		inline internal_::Service* impl() const;
	private:
		internal_::Service* pimpl;
	};

}