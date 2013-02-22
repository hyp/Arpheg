#pragma once

#include "../core/math.h"
#include "../core/bytes.h"

namespace input {

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

	//
	struct Binding {
		uint32 kind;
		uint32 data;
	};

	class Service {
	public:
		Service();
		~Service();
		void servicePreStep();
		void servicePostStep();

		//Capabilites and Devices
		inline bool hasPhysicalKeyboard() const;
		inline bool hasMouse() const;
		inline bool hasMultitouch() const;

		//Keyboard
		void pressKey  (uint32 keyCode);
		void releaseKey(uint32 keyCode);
		void emitCharacterEvent(uint32 character);
		inline bool isPressed(uint32 keyCode) const;
		inline core::Bytes text() const;

		//Mouse
		void emitMouseEvent(int32 x,int32 y,uint32 action = 0,uint32 button = 0);
		inline bool isMouseButtonPressed(uint32 button) const;
		inline bool isMouseButtonReleased(uint32 button) const;
		inline bool isMouseButtonDoubleClicked(uint32 button) const;
		inline vec2i cursorPosition() const;
		void   moveCursor(vec2i position);

		//Touch
		inline touch::Event* touchEvents();
		inline touch::Event* touchEventsEnd();
		touch::Event* getTouchEvent(int32 id);

		//Bindings
		void waitForEventBinding();

	private:
		enum {
			HasTouch = 0x1,
			HasMultiTouch = 0x2,
			HasMouse = 0x4,
			HasPhysicalKeyboard = 0x8,

			AwaitingBinding = 0x80,
		};
		uint32       caps;
		uint32       keyMap[256/32];
		vec2i        cursorPosition_;
		uint8        buttons[mouse::kMaxButtons];
		touch::Event touchEvents_[touch::kMaxPoints];
		uint32       textInputOffset;
		uint8        textInputBuffer[32];
	};

	inline touch::Event* Service::touchEvents()     { return touchEvents_; }
	inline touch::Event* Service::touchEventsEnd()  { return touchEvents_ + touch::kMaxPoints; }
	inline bool Service::isPressed(uint32 keyCode) const {
		enum { BitsPerUnit = sizeof(keyMap[0])*8 };
		return ( keyMap[(keyCode&0xFF)/BitsPerUnit]&(1<<(keyCode%BitsPerUnit)) ) != 0;
	}
	inline bool Service::hasPhysicalKeyboard() const{
		return (caps&HasPhysicalKeyboard)!=0;
	}
	inline bool Service::hasMouse() const{
		return (caps&HasMouse)!=0;
	}
	inline bool Service::hasMultitouch() const{
		return (caps&HasMultiTouch)!=0;
	}
	inline bool Service::isMouseButtonPressed(uint32 button) const {
		return buttons[button] == mouse::Pressed;
	}
	inline bool Service::isMouseButtonReleased(uint32 button) const {
		return buttons[button] == mouse::Released;
	}
	inline bool Service::isMouseButtonDoubleClicked(uint32 button) const  {
		return buttons[button] == mouse::DoubleClicked;
	}
	inline vec2i Service::cursorPosition() const {
		return cursorPosition_;
	}
	inline core::Bytes Service::text() const {
		return core::Bytes((void*)textInputBuffer,(void*)(textInputBuffer + textInputOffset));
	}
}
