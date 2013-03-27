#pragma once

#include "../core/memoryTypes.h"
#include "../input/events.h"
#include "types.h"

namespace ui {
namespace events {

struct Draw {
	rendering::ui::Service* renderer;
	uint32 layerId;
	core::BufferAllocator* glyphExtractionBuffer;
	vec2i position; vec2i size;
	
	inline vec2i innerMin() const;
	inline vec2i innerMax() const;
};
inline vec2i Draw::innerMin() const { return position; }
inline vec2i Draw::innerMax() const { return position+size; }

typedef input::events::Mouse Mouse;
typedef input::events::MouseButton MouseButton;
typedef input::events::MouseWheel MouseWheel;
typedef input::events::Key Key;
typedef input::events::Joystick Joystick;
typedef input::events::Touch Touch;

struct Layout {
	const Widget* parent;
	vec2i runningPosition;
};

inline bool contained(vec2i point,vec2i size) { return point.x <= size.x && point.y <= size.y; }

} }