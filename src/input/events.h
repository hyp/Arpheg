#pragma once

#include "types.h"

namespace input {
namespace events {

struct Event {};
struct Mouse: Event {
	vec2f position;
};
struct MouseButton: Mouse {
	uint32 button;
	mouse::ButtonState action;
};
struct MouseWheel: Mouse {
	int32 delta;
};
struct Key: Event {
	uint32 key;
};
struct Joystick: Event {
};
struct Touch: Event {

};

class IHandler {
public:
	virtual void onMouseMove(const events::Mouse& ev) = 0;
	virtual void onMouseButton(const events::MouseButton& ev) = 0;
	virtual void onMouseWheel(const events::MouseWheel& ev) = 0;
	virtual void onKey(const events::Key& ev) = 0;
	virtual void onJoystick(const events::Joystick& ev) = 0;
	virtual void onTouch(const events::Touch& ev) = 0;
};

} }
