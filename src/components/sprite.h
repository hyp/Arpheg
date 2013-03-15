#pragma once

#include "../resources/types.h"

namespace components {
	struct Sprite {
		data::Sprite* sprite;
		uint32 currentFrame;
	};
}
