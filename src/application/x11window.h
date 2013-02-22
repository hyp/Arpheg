#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

namespace application {
	struct X11Window {
		Display* display;
		Window   win;
		Colormap cmap;
		GLXFBConfig bestFbc;
		GLXContext ctx;
		Atom wmDeleteMessage;
	};
}
