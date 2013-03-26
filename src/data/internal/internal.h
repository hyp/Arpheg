#pragma once

#include "../../core/bytes.h"
#include "../../core/memory.h"
#include "../../core/bufferArray.h"
#include "../../rendering/types.h"
#include "../types.h"

namespace data {
namespace internal_ {
	typedef void (*ReleaseFunc)(rendering::Service*,void*);
	
	//This is used primarily for debugging purposes
	struct ResourceName {
		enum { kMaxLength = 32 };
		char string[kMaxLength];

		void operator =(const char* str);
		void operator =(core::Bytes str);
	};
	union RenderResourceDat {
		rendering::Buffer buffer;
		rendering::Mesh mesh;
		rendering::Texture2D texture2D;
		rendering::Shader shader;
		rendering::Pipeline pipeline;
		rendering::Sampler sampler;
	};
	struct RenderResource {
		enum {
			Buffer,Mesh,Texture2D,Shader,Pipeline,Sampler
		};
		ReleaseFunc func;
		RenderResourceDat obj;
	};
	struct ResourceStackSection {
		uint32 offset;
	};
	struct ResourceBundle {
		//a map of string ids to objects
		
		ResourceStackSection renderObjects;

	#ifndef ARPHEG_DATA_NO3D
		ResourceStackSection submeshes;
	#endif
		ResourceStackSection pipelines;
		ResourceStackSection objects;
		ResourceName name;
		
		//Storage for the object which maps id's to pointers
		uint8 mappingStorage[128];
	};


	class ObjectStack: public core::BufferAllocator {
	public:
		ObjectStack(size_t size = 1024*256);
		inline ResourceStackSection stackSection() const {
			ResourceStackSection s = { uint32(size()) };
			return s;
		}
		inline void release(ResourceStackSection start){
			if(auto diff = size() - size_t(start.offset)) this->deallocateFromTop(diff);
		}
	};

	// Assumptions:
	// Buffer's size will no exceed uint32 (4GiB)
	template<typename T>
	class ResourceStack: public core::BufferAllocator {
	public:
		ResourceStack(uint32 count= 128) : core::BufferAllocator(sizeof(T)*count,nullptr,core::BufferAllocator::GrowOnOverflow) {
		}
		inline ResourceStackSection stackSection() const {
			ResourceStackSection s = { uint32(size()) };
			return s;
		}
		inline T* createNew() { return core::bufferArray::allocate<T>(*this); }

		template<typename F>
		inline void foreach(F& f,ResourceStackSection start){
			assert(start.offset <= size());
			auto begin = (T*)( bufferBase() + start.offset );
			auto end   = core::bufferArray::end<T>(*this);
			for(;begin<end;++begin) f(*begin);
		}

		inline void release(ResourceStackSection start){
			if(auto diff = size() - size_t(start.offset)) this->deallocateFromTop(diff);
		}
	};



	class Service {
	public:
		Service();
		~Service();
	
		ResourceBundle* currentBundle;//The bundle in use for id lookup
		
	#ifndef ARPHEG_DATA_NO3D
		//A stack of submeshes
		ResourceStack<SubMesh> submeshes;
	#endif
		ResourceStack<Pipeline> pipelines;
		
		ObjectStack objects;
		//A stack of shading programs
		//ResourceStack<rendering::Pipeline> pipelines;
		//A stack of shaders
		//ResourceStack<rendering::Shader> shaders;

		//A stack of resource bundles
		ResourceStack<ResourceBundle> bundles;
	
		//A stack of unique rendering objects which are released when a bundle is released
		ResourceStack<RenderResource> renderObjects;



		void releaseTopBundle();
		void releaseBundles();
		ResourceBundle* newBundle(const char* name);
		BundleID loadBundle(const char* filename,const char* id);
		BundleID getBundle(const char* id,bool optional);
		void endBundle();
		void loadTextBundle(const char* path,ResourceBundle* bundle,core::Bytes bytes);
		
		void mapPointerToID(ResourceBundle* bundle,void* ptr,core::Bytes id);
		void* getResourceFromID(ResourceBundle* bundle,core::Bytes id);
		void* getResourceFromID(const char* id);
		void* getResourceFromID(BundleID bundle,const char* id,bool optional);
		
		
#ifndef ARPHEG_DATA_NO3D
		SubMesh* newSubMesh();
#endif
		Pipeline* newPipeline();

		template<typename T> inline T* allocateObject() {
			return (T*) objects.allocate(sizeof(T),alignof(T));
		}
		template<typename T> inline T* allocateObject(size_t extras) {
			return (T*) objects.allocate(sizeof(T)+extras,alignof(T));
		}

		// Register rendering objects for automatic release when bundle is released
		void renderingBuffer(rendering::Buffer buffer);
		void renderingMesh(const rendering::Mesh& mesh);
		void renderingTexture2D(rendering::Texture2D texture2D);
		void registerShader(rendering::Shader shader);
		void registerPipeline(rendering::Pipeline pipeline);
	};


} }