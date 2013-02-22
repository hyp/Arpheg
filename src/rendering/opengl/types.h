#pragma once

namespace rendering {
	//Strong typedef
	struct Buffer {
		enum Type {
			Vertex, Index, Constant
		};
		uint32 id;

		inline bool isNull() { return id == 0; }
		inline bool makeNull() { id = 0; }
	};
	struct TextureBuffer {
		uint32 id;
	};
	struct Texture1D {
		uint32 id;
	};
	struct Texture2D {
		uint32 id;
	};
	struct Texture2DArray {
		uint32 id;
	};
	struct Texture3D {
		uint32 id;
	};

	struct Shader       {
		enum Type {
			Vertex,Pixel,Geometry
		};
		uint32 id;
	};
	struct Pipeline  {
		struct Constant {
			int32 location;
			uint8 shaderType,coreTypeId;
			uint16 count;
			const char* name;

			Constant(const char* name);
		};
		uint32 id;
		uint32 inputDescriptorCount;
		CoreTypeDescriptor* inputDescriptor;
	};
	struct Sampler {
		uint32 id;
	};
	struct RenderTarget {
		uint32 id;
		uint32 datId;
	};
}
