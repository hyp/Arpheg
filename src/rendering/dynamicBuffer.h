#pragma once

#include "../core/bytes.h"
#include "types.h"

namespace rendering {

	struct DynamicConstantBuffer {
		DynamicConstantBuffer();
		~DynamicConstantBuffer();
		void update(core::Bytes data);

		Buffer buffer;
	};

	struct DynamicBufferTexture {

		DynamicBufferTexture();
		~DynamicBufferTexture();
		void update(texture::Format format,core::Bytes data);

		Buffer buffer;
		TextureBuffer textureView;
	};
}