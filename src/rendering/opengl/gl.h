#pragma once

#include "../../core/platform.h"
#ifdef ARPHEG_PLATFORM_MARMALADE
	#include <IwGL.h>
#else

	#ifdef ARPHEG_PLATFORM_WIN32
		#include "gl3/gl3.h"
		#include "api.h"
		#include "gl/wglext.h"
	#elif defined(ARPHEG_PLATFORM_LINUX)

		#define GL_GLEXT_PROTOTYPES
		#define GLX_GLXEXT_PROTOTYPES

		#include <X11/Xlib.h>
		#include <GL/gl.h>
		#include <GL/glx.h>

		#include "gl/glxext.h"
		#include "gl3/gl3.h"
	#endif

#endif

//#define CHECK_GL(code) code; if(glGetError()!=0) assert(false);
#define CHECK_GL(code) code