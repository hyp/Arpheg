#include "../core/platform.h"
#include "../services.h"
#include "application.h"

#if defined(ARPHEG_PLATFORM_X11)
	#include "x11window.h"
#endif

namespace application {

void Service::quit() {
	quit_ = true;
}
void Service::activate(bool active) {
	if(active != active_){
		services::logging()->trace(active? "Application activated" : "Application deactivated");
	}
	active_ = active;
}
Service::Service() {
	quit_ = false;
	active_ = true;
	frameID_ = 0;
}
Service::~Service() {
}

#ifdef ARPHEG_PLATFORM_WIN32
}
#include <Windowsx.h>


//Win32 event dispatch
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	uint32 mouseKey,mouseAction;
	using namespace input;

	switch (msg) {
		case WM_KEYDOWN:
			services::input()->pressKey(uint32(wParam));
			break;
		case WM_KEYUP:
			services::input()->releaseKey(uint32(wParam));
			break;
		case WM_CHAR:
			services::input()->emitCharacterEvent(uint32(wParam));
			break;
		case WM_LBUTTONDOWN: 
			mouseKey = mouse::Left;
			mouseAction = mouse::Pressed;
			goto mouseButton;
		case WM_LBUTTONUP: 
			mouseKey = mouse::Left;
			mouseAction = mouse::Released;
			goto mouseButton;
		case WM_RBUTTONDOWN:
			mouseKey = mouse::Right;
			mouseAction = mouse::Pressed;
			goto mouseButton;
		case WM_RBUTTONUP:
			mouseKey = mouse::Left;
			mouseAction = mouse::Released;
			goto mouseButton;
		case WM_MBUTTONDOWN:
			mouseKey = mouse::Middle;
			mouseAction = mouse::Pressed;
			goto mouseButton;
		case WM_MBUTTONUP:
			mouseKey = mouse::Middle;
			mouseAction = mouse::Released;
			goto mouseButton;
		case WM_XBUTTONDOWN:
			mouseKey = mouse::Middle+GET_XBUTTON_WPARAM(wParam);
			mouseAction = mouse::Pressed;
			goto mouseButton;
		case WM_XBUTTONUP:
			mouseKey = mouse::Middle+GET_XBUTTON_WPARAM(wParam);
			mouseAction = mouse::Released;
mouseButton:
			services::input()->emitMouseEvent(0,0,mouseAction,mouseKey);
			return FALSE;
		//case WM_MOUSEWHEEL:

		case WM_MOUSEMOVE:
			services::input()->emitMouseEvent(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			break;
		case WM_SIZE:
			if(wParam == SIZE_MINIMIZED){
				services::application()->activate(false);
				break;
			}
			services::application()->activate(true);
			services::application()->mainWindow()->resize(vec2i(LOWORD(lParam),HIWORD(lParam)));
			break;
		case WM_ACTIVATE:
			services::application()->activate(wParam != WA_INACTIVE);
			break;
		case WM_SHOWWINDOW:
			services::application()->activate(wParam == TRUE);
			break;
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return FALSE;
}

namespace application {
#endif

void Service::servicePreStep() {
	frameID_ ++;

#ifdef ARPHEG_PLATFORM_WIN32
	//Win32 event dispatch
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			quit_ = true;
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#elif defined(ARPHEG_PLATFORM_X11)
	//X11 event dispatch
	auto data = (X11Window*)mainWindow()->handle();

	XEvent ev;
	int eventCount = XEventsQueued(data->display,QueuedAlready);
	for(int i = 0;i < eventCount;++i){
		//if(i==eventCount) XPeekEvent(data->display, &ev);
		//else 
		XNextEvent(data->display,&ev);

		switch(ev.type){
			case KeyPress:
			case KeyRelease:{
				uint32 keyCode = 0;
				char asciiCode[ 32 ];
				KeySym keySym;

				int len = XLookupString(&ev.xkey,asciiCode,sizeof(asciiCode)-1,&keySym,NULL);
				if(len > 0){
					keyCode = asciiCode[0];
				} else {
					using namespace input::keyboard;
					switch( keySym )
                    {
					case XK_Escape: keyCode = Escape; break;
					case XK_Control_L: keyCode = Control; break;
					case XK_Return: keyCode = Return; break;
					case XK_Shift_L:   keyCode = Shift; break;
					case XK_space: keyCode = Space; break;
					case XK_Tab: keyCode = Tab; break;

                    case XK_F1:     keyCode = F1;     break;
                    case XK_F2:     keyCode = F2;     break;
                    case XK_F3:     keyCode = F3;     break;
                    case XK_F4:     keyCode = F4;     break;
                    case XK_F5:     keyCode = F5;     break;
                    case XK_F6:     keyCode = F6;     break;
                    case XK_F7:     keyCode = F7;     break;
                    case XK_F8:     keyCode = F8;     break;
                    case XK_F9:     keyCode = F9;     break;
                    case XK_F10:    keyCode = F10;    break;
                    case XK_F11:    keyCode = F11;    break;
                    case XK_F12:    keyCode = F12;    break;

                    case XK_KP_Left:
                    case XK_Left:   keyCode = Left;   break;
                    case XK_KP_Right:
                    case XK_Right:  keyCode = Right;  break;
                    case XK_KP_Up:
                    case XK_Up:     keyCode = Up;     break;
                    case XK_KP_Down:
                    case XK_Down:   keyCode = Down;   break;

                    case XK_KP_Prior:
                    case XK_Prior:  keyCode = Prior; break;
                    case XK_KP_Next:
                    case XK_Next:   keyCode = Next; break;
                    case XK_KP_Home:
                    case XK_Home:   keyCode = Home;   break;
                    case XK_KP_End:
                    case XK_End:    keyCode = End;    break;
                    case XK_KP_Insert:
                    case XK_Insert: keyCode = Insert; break;

                    case XK_KP_Delete:  keyCode = Delete;    break;
                    }
				}
				auto input = services::input();
				if(ev.type == KeyPress) input->pressKey(keyCode);
				else input->releaseKey(keyCode);
				}
				break;
			case ButtonPress: {
				int button = ev.xbutton.button - 1;
				services::input()->emitMouseEvent(0,0,input::mouse::Pressed,button);
				}
				break;
			case ButtonRelease: {
				int button = ev.xbutton.button - 1;
				services::input()->emitMouseEvent(0,0,input::mouse::Released,button);
				}
				break;
			case MotionNotify:
				services::input()->emitMouseEvent(ev.xmotion.x,ev.xmotion.y);
				break;
			case ConfigureNotify:
				activate(true);
				mainWindow()->resize(vec2i(ev.xconfigure.width,ev.xconfigure.height));
				break;
			case VisibilityNotify:
				activate(ev.xvisibility.state != VisibilityFullyObscured);
				break;
			case MapNotify:
				activate(true);
				break;
			case UnmapNotify:
				activate(false);
				break;
			case DestroyNotify:
				quit_ = true;
				break;
			case ClientMessage:
				if (ev.xclient.data.l[0] == data->wmDeleteMessage)
					quit_ = true;
				break;
			default:
				break;
		}
	}
#endif
}
void Service::servicePostStep() {
#ifdef ARPHEG_PLATFORM_MARMALADE
	if(s3eDeviceCheckQuitRequest()) quit_ = true;
	if (s3eKeyboardGetState(s3eKeyAbsBSK) & S3E_KEY_STATE_DOWN) // Back key is used to exit on some platforms
		quit_ = true;
	if(!quit_) s3eDeviceYield(0);
#endif
}

}