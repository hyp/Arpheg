#include <string.h>

#include "../core/platform.h"
#include "../core/assert.h"
#include "../core/utf.h"
#include "../core/bufferArray.h"
#include "../services.h"
#include "input.h"
#include "events.h"

#ifdef ARPHEG_PLATFORM_X11
#include "../application/x11window.h"
#endif

namespace input {

#ifdef ARPHEG_PLATFORM_MARMALADE

//touch event handlers
void onMultiTouchButton(s3ePointerTouchEvent* e,void* ud){
	auto service = (Service*)ud;
	if(auto touch = service->getTouchEvent(e->m_TouchID)){
		touch->position.x = e->m_x;
		touch->position.y = e->m_y;
		touch->isActive = e->m_Pressed != 0;
	}
}
void onMultiTouchMotion(s3ePointerTouchMotionEvent* e,void* ud){
	auto service = (Service*)ud;
	if(auto touch = service->getTouchEvent(e->m_TouchID)){
		touch->position.x = e->m_x;
		touch->position.y = e->m_y;
	}
}
void onSingleTouchButton(s3ePointerEvent* e,void* ud){
	auto service = (Service*)ud;
	service->touchEvents()[0].position.x = e->m_x;
	service->touchEvents()[0].position.y = e->m_y;
	service->touchEvents()[0].isActive = e->m_Pressed != 0;
}
void onSingleTouchMotion(s3ePointerMotionEvent* e,void* ud){
	auto service = (Service*)ud;
	service->touchEvents()[0].position.x = e->m_x;
	service->touchEvents()[0].position.y = e->m_y;
}

#endif

Service::Service() : emmitedEvents_(1024,core::memory::globalAllocator(),core::BufferAllocator::GrowOnOverflow) {
	for(auto touch = touchEvents(),end = touchEventsEnd();touch < end;++touch) touch->isActive = 0;
	memset(keyMap,0,sizeof(keyMap));
	memset(buttons,0,sizeof(buttons));
	caps = HasPhysicalKeyboard|HasMouse;
	textInputOffset = 0;

#ifdef ARPHEG_PLATFORM_MARMALADE
	//caps = 0;
	auto hasMultitouch = s3ePointerGetInt(S3E_POINTER_MULTI_TOUCH_AVAILABLE) ? true : false;
	if (hasMultitouch) {
		s3ePointerRegister(S3E_POINTER_TOUCH_EVENT, (s3eCallback)onMultiTouchButton, this);
		s3ePointerRegister(S3E_POINTER_TOUCH_MOTION_EVENT, (s3eCallback)onMultiTouchMotion, this);
		caps |= HasMultiTouch;
	} else {
		s3ePointerRegister(S3E_POINTER_BUTTON_EVENT, (s3eCallback)onSingleTouchButton, this);
		s3ePointerRegister(S3E_POINTER_MOTION_EVENT, (s3eCallback)onSingleTouchMotion, this);
	}
#endif
}
Service::~Service(){
#ifdef ARPHEG_PLATFORM_MARMALADE
	auto hasMultitouch = s3ePointerGetInt(S3E_POINTER_MULTI_TOUCH_AVAILABLE) ? true : false;
	if (hasMultitouch) {
		s3ePointerUnRegister(S3E_POINTER_TOUCH_EVENT, (s3eCallback)onMultiTouchButton);
		s3ePointerUnRegister(S3E_POINTER_TOUCH_MOTION_EVENT, (s3eCallback)onMultiTouchMotion);
	}
	else {
		s3ePointerUnRegister(S3E_POINTER_BUTTON_EVENT, (s3eCallback)onSingleTouchButton);
		s3ePointerUnRegister(S3E_POINTER_MOTION_EVENT, (s3eCallback)onSingleTouchMotion);
	}
#endif
}
void Service::servicePreStep() {
#ifdef ARPHEG_PLATFORM_MARMALADE
	s3eKeyboardUpdate();
	s3ePointerUpdate();
#endif
}
void Service::servicePostStep() {
	//Clear the mouse button events
	caps &= ~FlagMouseMoved;
	for(uint32 i = 0;i<mouse::kMaxButtons;++i){
		buttons[i] = 0;
	}
	textInputOffset = 0; //Clear any text input
}
void Service::pressKey  (uint32 keyCode) {
	assert(keyCode <= 255);
	enum { BitsPerUnit = sizeof(keyMap[0])*8 };
	//if(caps & AwaitingBinding)
	keyMap[(keyCode&0xFF)/BitsPerUnit] |= (1<<(keyCode%BitsPerUnit));
}
void Service::releaseKey(uint32 keyCode) {
	assert(keyCode <= 255);
	enum { BitsPerUnit = sizeof(keyMap[0])*8 };
	keyMap[(keyCode&0xFF)/BitsPerUnit] &= ~(1<<(keyCode%BitsPerUnit));
}
void Service::emitCharacterEvent(uint32 character) {
	if(textInputOffset >= (sizeof(textInputBuffer) - 6)) return;
	textInputOffset += core::utf8::encode(character,textInputBuffer+textInputOffset);
}
void Service::emitMouseEvent(int32 x,int32 y,uint32 action,uint32 button) {
	assert(button <= mouse::kMaxButtons);
	if(action == 0){
		cursorPosition_ = vec2i(x,y);
		caps |= FlagMouseMoved;
	}
	else {
		//if(caps & AwaitingBinding)
		buttons[button] = action;
	}
}
void   Service::moveCursor(vec2i position) {
#ifdef ARPHEG_PLATFORM_WIN32
	HWND hwnd = (HWND)services::application()->mainWindow()->handle();
	POINT ms = { position.x,position.y };
	ClientToScreen(hwnd,&ms);
	SetCursorPos(ms.x,ms.y);
#elif defined(ARPHEG_PLATFORM_X11)
	auto data = (application::X11Window*)services::application()->mainWindow()->handle();
    XWarpPointer(data->display,None,data->win,0, 0, 0, 0,position.x, position.y);
    /* Make the warp visible immediately. */
    XFlush( data->display );
#endif
}
touch::Event* Service::getTouchEvent(int32 id){
    touch::Event* inactive = nullptr;
    for(auto touch = touchEvents(),end = touchEventsEnd();touch < end;++touch){
        if (id == touch->id)
            return touch;
        if (!touch->isActive)
            inactive = touch;
    }
    if (inactive) inactive->id = id;
    return inactive;
}

void Service::waitForEventBinding() {
	caps |= AwaitingBinding;
}

void Service::handleEvents(events::IHandler* handler) {
	//TODO key, touch, joy
	auto mouse = vec2i(cursorPosition_.x,cursorPosition_.y);
	if(caps & FlagMouseMoved){
		events::Mouse ev;ev.position = mouse; handler->onMouseMove(ev);
	}

	events::MouseButton mb;
	mb.position = mouse;
	for(uint32 i = 0;i<mouse::kMaxButtons;++i){
		if(buttons[i]){
			mb.button = i;
			mb.action = mouse::ButtonState(buttons[i]);
			handler->onMouseButton(mb);
		}
	}
}

}
