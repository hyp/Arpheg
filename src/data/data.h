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
		BundleID loadBundle(const char* filename,ID id);
		BundleID bundle(ID id,bool optional = false);
		
		//raw rendering resources without extra cruff
		rendering::Texture2D      texture2D(ID id);
		rendering::Texture2DArray texture2DArray(ID id);
		rendering::Sampler sampler(ID id);

		//Selects assets from the current bundle
		Pipeline* pipeline(ID id);
		SubMesh*  submesh(ID id);
		Mesh* mesh(ID id);
		animation::Animation* animation(ID id);
		Font* font(ID id);
		Sprite* sprite(ID id);


		rendering::Texture2D      texture2D(BundleID bundle,ID id,bool optional = false);
		rendering::Texture2DArray texture2DArray(BundleID bundle,ID id,bool optional = false);
		rendering::Sampler sampler(BundleID bundle,ID id,bool optional = false);
		Pipeline* pipeline(BundleID bundle,ID id,bool optional = false);
		SubMesh*  submesh(BundleID bundle,ID id,bool optional = false);
		Mesh* mesh(BundleID bundle,ID id,bool optional = false);
		animation::Animation* animation(BundleID bundle,ID id,bool optional = false);
		Font* font(BundleID bundle,ID id,bool optional = false);
		Sprite* sprite(BundleID bundle,ID id,bool optional = false);

		inline internal_::Service* impl() const;
	private:
		internal_::Service* pimpl;
	};

}