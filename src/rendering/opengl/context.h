#pragma once

#include "../../core/math.h"

namespace rendering {
namespace opengl {

namespace extensions {
#ifdef PLATFORM_RENDERING_GLES
	enum {
		OES_texture_npot = 0x1,
		OES_element_index_uint = 0x2,
	};
#else
	enum {
		ARB_debug_output = 0x1,
		AMD_pinned_memory = 0x2,
		ARB_vertex_type_2_10_10_10_rev = 0x4,
		EXT_direct_state_access = 0x8,
	};
#endif
};
namespace support {
	enum {
		GL3_geometry_shaders = 1,
		GL3_sampler_objects = 2,
		GL3_uniform_buffer_objects = 4,
		GL3_hardware_instancing = 8,
		GL3_texture_buffer_objects = 0x10,
		GL3_ms_textures = 0x20,
		GL4_tesselation = 0x100,
	};
};

class Context {
public:
	Context();
	~Context();

	vec2i frameBufferSize();
	inline vec2i version() const;
	
	void swapBuffers(bool vsync = false);
	bool extensionSupported(uint32 extension);
	void* getProcAddress(const char* str);

	// API gimmicks.
	inline uint32 constantBufferOffsetAlignment() const;

	// API support
	inline bool geometryShadersSupported() const;
	inline bool tesselationSupported()  const;

	inline bool samplersSupported() const;
	inline bool textureBuffersSupported() const;
	inline bool constantBuffersSupported() const;
	inline bool hardwareInstancingSupported() const;

	inline bool multisampledRenderTargetSupported() const;

private:
	uint32 apiSupport;
	uint32 extCheck,extSupport;
	uint32 uboOffsetAlignment_;
	void* data[2];
	vec2i version_;
	bool vsync_;
	void checkApiSupport();
};

inline vec2i Context::version() const { return version_; }
inline uint32 Context::constantBufferOffsetAlignment() const { return uboOffsetAlignment_; }
inline bool Context::geometryShadersSupported() const { return (apiSupport & support::GL3_geometry_shaders)!=0; }
inline bool Context::tesselationSupported() const { return (apiSupport & support::GL4_tesselation)!=0; } 

inline bool Context::samplersSupported() const { return (apiSupport & support::GL3_sampler_objects)!=0; } 
inline bool Context::textureBuffersSupported() const { return (apiSupport & support::GL3_texture_buffer_objects)!=0; } 
inline bool Context::constantBuffersSupported() const { return (apiSupport & support::GL3_uniform_buffer_objects)!=0; } 
inline bool Context::hardwareInstancingSupported() const { return (apiSupport & support::GL3_hardware_instancing)!=0; } 

inline bool Context::multisampledRenderTargetSupported() const { return (apiSupport & support::GL3_ms_textures)!=0; } 


} }