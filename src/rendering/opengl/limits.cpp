#include "../../core/assert.h"
#include "../../core/allocatorNew.h"
#include "gl.h"
#include "rendering.h"

namespace rendering {

	static inline int getInt(GLenum param){
		int x = 0;
		glGetIntegerv(param,&x);
		return x;
	}
	void getLimits(){
		Limits limits;
		limits.maxTexture2DSize.x = limits.maxTexture2DSize.y = getInt(GL_MAX_TEXTURE_SIZE);
		limits.maxTexture3DSize = getInt(GL_MAX_3D_TEXTURE_SIZE);
		limits.maxTextureArrayLayers = getInt(GL_MAX_ARRAY_TEXTURE_LAYERS);
		limits.maxConstantBufferSize = (size_t)getInt(GL_MAX_UNIFORM_BLOCK_SIZE);
	}
}