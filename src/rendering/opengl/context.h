#pragma once

#include "../../core/math.h"

namespace rendering {
namespace opengl {

namespace extensions {
#ifdef PLATFORM_RENDERING_GLES
	enum {
		OES_texture_npot = 1,
		OES_element_index_uint = 2,
	};
#endif
};

class Context {
public:
	Context();
	~Context();

	vec2i frameBufferSize();
	inline vec2i version() const;
	
	void swapBuffers(bool vsync = false);
	bool extensionSupported(uint32 extension);
private:
	uint32 extCheck,extSupport;
	void* data[2];
	vec2i version_;
	bool vsync_;
};

inline vec2i Context::version() const { return version_; }

} }