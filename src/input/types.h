#pragma once

#include "../core/math.h"
#include "../core/bytes.h"

namespace input {

	namespace events {
		class IHandler;
	}

	namespace keyboard {
		enum {
			Back = 0x08,
			Tab  = 0x09,

			Clear = 0x0C,
			Return = 0x0D,

			Shift = 0x10,
			Control = 0x11,
			Menu = 0x12,
			Pause = 0x13,
			
			Escape = 0x1B,
			
			Space = 0x20,
			Prior = 0x21,
			Next  = 0x22,
			End   = 0x23,
			Home  = 0x24,
			Left  = 0x25,
			Up    = 0x26,
			Right = 0x27,
			Down  = 0x28,
			Select = 0x29,
			Print  = 0x2A,
			Insert  = 0x2D,
			Delete  = 0x2E,
			Help    = 0x2F,

			Sleep =  0x5F,

			Numpad0 = 0x60,
			Numpad1,
			Numpad2,
			Numpad3,
			Numpad4,
			Numpad5,
			Numpad6,
			Numpad7,
			Numpad8,
			Numpad9,
			
			F1 = 0x70,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			F13,
			F14,
			F15,
			F16
		};
	}

	namespace touch {
		struct Event {
			vec2i position;
			int32 id,isActive;
		};

		enum {
			kMaxPoints = 10
		};
	}

	namespace mouse {
		enum {
			kMaxButtons = 16
		};

		enum ButtonState {
			Pressed = 1,Released,DoubleClicked
		};

		enum Button {
			Left,Right,Middle
		};
	}
	
	namespace joystick {
		enum {
			kMaxButtons = 32,
			KMaxDevices = 4
		};

		struct Device {

		};
	}

	//TODO
	struct Binding {
		uint32 kind;
		uint32 data;
	};

	class Service;
}
