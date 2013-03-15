#pragma once

#include "../core/math.h"
#include "flexibleRectangle.h"
#include "transform2D.h"

namespace components {
	namespace eventSlots {
		struct ActiveKey {
			enum Event {
				None = 0, Pressed
			};
			uint16 key;
			uint16 altKey;
			uint32 e;
			
			inline ActiveKey() : key(0),altKey(0),e(0) {}
			inline bool isPressed() { return e == Pressed; }

			static void update(ActiveKey* begin,size_t count);
		};
		struct ActiveRectangle {
			enum Event {
				None = 0, MouseLeft, MouseRight, Touch
			};
			FlexibleRectangle rectangle;
			uint32 e;

			inline ActiveRectangle(): e(0),rectangle(vec2i(0,0),vec2i(0,0)) { }
			inline bool isTouched() { return e == Touch; }

			static void update(ActiveRectangle* begin,size_t count);
		};
	}
}
