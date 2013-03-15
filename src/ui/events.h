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
	vec2f position; vec2f size;
	
	inline vec2f innerMin() const;
	inline vec2f innerMax() const;
};
inline vec2f Draw::innerMin() const { return position; }
inline vec2f Draw::innerMax() const { return position+size; }

typedef input::events::Mouse Mouse;
typedef input::events::MouseButton MouseButton;
typedef input::events::MouseWheel MouseWheel;
typedef input::events::Key Key;
typedef input::events::Joystick Joystick;
typedef input::events::Touch Touch;

inline bool contained(vec2f point,vec2f size) { return point.x <= size.x && point.y <= size.y; }

} }