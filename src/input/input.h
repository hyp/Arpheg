#pragma once

#include "types.h"

namespace input {

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

		//Events
		void handleEvents(events::IHandler* handler); 

	private:
		enum {
			HasTouch = 0x1,
			HasMultiTouch = 0x2,
			HasMouse = 0x4,
			HasPhysicalKeyboard = 0x8,

			AwaitingBinding = 0x80,

			FlagMouseMoved = 0x100,
		};
		uint32       caps;
		uint32       keyMap[256/32];
		vec2i        cursorPosition_;
		uint8        buttons[mouse::kMaxButtons];
		touch::Event touchEvents_[touch::kMaxPoints];
		uint32       textInputOffset;
		uint8        textInputBuffer[32];
		core::BufferAllocator emmitedEvents_;
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
