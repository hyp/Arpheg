#pragma once

namespace rendering {
	//Uses strong typedefs

	// A buffer holds information
	struct Buffer {
		enum Type {
			Vertex, Index, Constant
		};
		uint32 id;

		inline bool isNull() { return id == 0; }
		inline void makeNull() { id = 0; }
		static inline Buffer nullBuffer() { Buffer result = { 0 }; return result; }

		struct Mapping {
			void* data;
			uint32 id;
			Type  type;
		};
	};

	// A mesh is simply a vertex buffer and an index buffer, 
	// however it may also store some optimization information for specific API,
	// like VAO for OpenGL, 
	// and therefore it may bind and render faster than two separate buffer binds.
	struct Mesh {
		Buffer vbo;
		Buffer ibo;
		uint32 vao;
	};

	struct Texture1D {
		uint32 id;
		uint32 type;
	};
	struct Texture2D {
		uint32 id;
		uint32 type;

		static inline Texture2D null() { Texture2D result = { 0,0 }; return result; }
		inline bool operator ==(Texture2D other) const { return id == other.id; }
	};
	struct Texture2DArray {
		uint32 id;
		uint32 type;
	};
	struct Texture3D {
		uint32 id;
		uint32 type;
	};
	struct TextureBuffer {
		uint32 id;
		uint32 type;
	};

	struct Sampler {
		uint32 id;
	};
	struct RenderTarget {
		uint32 id;
		uint32 datId;
	};
	struct Shader {
		enum Type {
			Vertex,Pixel,Geometry,TesselationControl,TesselationEvaluation,MaxTypes
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

		//GLSL specific extra information for program linking (GLSL 3.3 layout)
		struct GLSLLayout {
			const char** vertexAttributes;
			uint32 vertexAttributeCount;
			const char** pixelAttributes;
			uint32 pixelAttributeCount;

			GLSLLayout();
		};
	
		uint32 id;

		static inline Pipeline nullPipeline() { Pipeline result = { 0 }; return result; }
		inline bool operator ==(Pipeline other) const { return id == other.id; }
	};

}
