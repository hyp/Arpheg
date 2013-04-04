#pragma once

#include "../../../core/memoryTypes.h"
#include "../../../core/math.h"
#include "../../../core/bytes.h"
#ifdef ARPHEG_BUILD_DATA_LIB
#include "../../../core/dynamicLibraryExport.h"
#endif
#include "../../types.h"
#include "../../../application/logging.h"

namespace data {
namespace intermediate {

struct Mesh {
	uint32 indexSize;
	core::Bytes vertices,indices;
	::data::SubMesh::Joint* joints;
	uint32 materialIndex;

	inline Mesh();
};

inline Mesh::Mesh() : vertices(nullptr,nullptr),indices(nullptr,nullptr),joints(nullptr) {
	indexSize = 0;
	materialIndex = 0;
}

namespace mesh {
	class Reader {
	public:

		//Useful for fine grained import control
		struct Options {
			bool optimize;
			bool leftHanded;
			uint32 maxBonesPerVertex;
			enum VertexFormat {
				Position,
				PositionNormal,
				PositionNormalTexcoord0,
				PositionNormalTangentTexcoord0,
				PositionNormalTangentBitangentTexcoord0,

				VertexWeights = 0x8000,
			};

			inline Options() : optimize(false),leftHanded(false),maxBonesPerVertex(4) {}
		};

#ifdef ARPHEG_BUILD_DATA_LIB
		void load(application::logging::Service* logger,core::Allocator* allocator,const char* name,const Options& options = Options());
#endif		

		virtual void processMesh(const Mesh* submeshes,const ::data::Mesh& mesh,uint32 vertexFormat) = 0;
		virtual void processMaterial(const char* name,const char** textures,size_t textureCount) = 0;
		virtual void processSkeletalAnimation(const char* name,const animation::Animation& animation) = 0;
	};

} } }

#ifdef ARPHEG_BUILD_DATA_LIB
extern "C" {
	ARPHEG_EXPORT void dataIntermediateMeshReaderLoad(void* reader,void* logger,void* allocator,const char* name,const void* options);
}
#else
extern "C" {
	typedef void* (*dataIntermediateMeshReaderLoadFunc)(void*,void*,void*,const char*,const void*);
}
#endif
