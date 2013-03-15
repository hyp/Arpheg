#include "../core/memory.h"
#include "../core/allocatorNew.h"
#include "types.h"

namespace resources {
	Sprite* Sprite::create(core::Allocator* allocator,uint32 frameCount,rendering::Texture2D texture,rendering::Buffer buffer) {
		auto sprite = (Sprite*)allocator->allocate(sizeof(Sprite) + frameCount*sizeof(Frame),alignof(Sprite));
		new(sprite) Sprite;
		sprite->texture = texture;
		sprite->buffer  = buffer;
		sprite->frameData = frameCount;
		return sprite;
	}
}
