#include "../services.h"
#include "../core/platform.h"
#include "../core/utf.h"
#include "application.h"

#ifdef ARPHEG_PLATFORM_WIN32
	extern LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#elif defined(ARPHEG_PLATFORM_X11)
	#include "x11window.h"
#endif

namespace application {

MainWindow::MainWindow(){
	createdFirst_ = false;
	created_ = false;
	handle_  = nullptr;
	size_    = vec2i(0,0);
}

#ifdef ARPHEG_PLATFORM_WIN32

static WNDCLASSEXW wcx;
void MainWindow::create(const char* title,vec2i size){
	HINSTANCE hInstance = (HINSTANCE) GetModuleHandle(0);

	if(!createdFirst_){
		//Create the window class
		memset(&wcx, 0, sizeof (wcx));
		wcx.cbSize = sizeof (wcx);
		wcx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcx.lpfnWndProc = reinterpret_cast<WNDPROC> (WindowProc);
		wcx.hInstance = hInstance;
		wcx.lpszClassName = L"ArhegWindowClass";
		wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcx.hCursor = LoadCursor(NULL, IDC_ARROW);

		RegisterClassExW(&wcx);
	}
	if(created_) DestroyWindow((HWND)handle_);

	if(size.x == 0) size.x = CW_USEDEFAULT;
	if(size.y == 0) size.y = CW_USEDEFAULT;

	WCHAR buffer[256] = {0};
	for(int i = 0;i<255 && ((*title) != '\0');++i){
		uint32 len = *title < 128? 1 : core::utf8::sequenceLength(*title);
		uint32 c   = core::utf8::decode(len,(const uint8*)title);
		buffer[i] = WCHAR(c);
		title+=len;
	}

	HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"ArhegWindowClass", buffer, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
		size.x, size.y, NULL, NULL, hInstance, NULL);
	if(!hwnd){
		services::logging()->fatal("Failed to create a window!");
	}
	ShowWindow(hwnd, SW_SHOW);

	RECT rect;
	GetClientRect(hwnd,&rect);
	size.x=rect.right -rect.left;
	size.y=rect.bottom-rect.top;

	handle_  = (void*)hwnd;
	created_ = true;
	size_ = size;

	if(!createdFirst_){
		services::postWindowCreation();
		createdFirst_ = true;
	}
}
MainWindow::~MainWindow(){
	if(created_)
		DestroyWindow((HWND)handle_);
}
void MainWindow::resize(vec2i size) {
	size_ = size;
}

#elif defined(ARPHEG_PLATFORM_X11)

void MainWindow::create(const char* title,vec2i size){
	auto logging = services::logging();

	//Open display
	Display *display;
	if (!(display = XOpenDisplay(0))) {
		logging->fatal("Failed to open X11 display!");
		return;
	}

	//Check GLX version
	int glx_major, glx_minor;
 
	 // FBConfigs were added in GLX version 1.3.
	 if ( !glXQueryVersion( display, &glx_major, &glx_minor ) || 
		( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) ) {
		logging->fatalSolution("Failed to create an OpenGL window - The GLX version is less than 1.3!","Please update your graphics card drivers!");
		return;
	}

	//Visual attributes
	int visual_attribs[] = {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , 24,
      GLX_STENCIL_SIZE    , 8,
      GLX_DOUBLEBUFFER    , True,
      //GLX_SAMPLE_BUFFERS  , 1,
      //GLX_SAMPLES         , 4,
      None
    };
	
	int fbcount = 0;
	GLXFBConfig *fbc = glXChooseFBConfig( display, DefaultScreen( display ), visual_attribs, &fbcount );
	if ( !fbc ) {
		logging->fatal("Couldn't get GLX FB configs!");
		return;
	}
	GLXFBConfig bestFbc = *fbc;

	XVisualInfo *vi = glXGetVisualFromFBConfig( display, bestFbc );
	if (!vi){
		logging->fatal("Couldn't choose GLX visual attributes!");
		return;
	}

	XFree( fbc );

	//Create window
	if(size.x == 0) size.x = 640;
	if(size.y == 0) size.y = 480;

	XSetWindowAttributes swa;
	Colormap cmap;
	swa.colormap = cmap = XCreateColormap( display,RootWindow( display, vi->screen ), vi->visual, AllocNone );
	swa.background_pixmap = None ;
	swa.border_pixel      = 0;
	swa.event_mask        = ExposureMask | VisibilityChangeMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | SubstructureNotifyMask |
    FocusChangeMask;
 
	Window win = XCreateWindow( display, RootWindow( display, vi->screen ), 
								0, 0, size.x, size.y, 0, vi->depth, InputOutput, 
								vi->visual, 
								CWBorderPixel|CWColormap|CWEventMask, &swa );
	if ( !win ) {
		logging->fatal("Failed to create an X11 window!");
		return;
	}
 
	// Done with the visual info data
	XFree( vi );

	XStoreName( display, win, title );
	XMapWindow( display, win );

	auto data = new X11Window();
	data->display = display;
	data->win = win;
	data->cmap = cmap;
	data->bestFbc = bestFbc;
	data->wmDeleteMessage = XInternAtom(data->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(data->display, data->win, &data->wmDeleteMessage, 1);

	handle_ = data;

	created_ = true;
	size_ = size;

	if(!createdFirst_){
		services::postWindowCreation();
		createdFirst_ = true;
	}
}
MainWindow::~MainWindow(){
	if(created_){
		auto data = (X11Window*)handle_;
		XDestroyWindow( data->display, data->win );
		XFreeColormap( data->display, data->cmap );
		XCloseDisplay( data->display );
		delete data;
	}
}
void MainWindow::resize(vec2i size) {
	size_ = size;
}

#endif

}