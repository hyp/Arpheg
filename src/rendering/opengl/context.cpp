#include <string.h>
#include "../../core/assert.h"
#include "../../core/bufferStringStream.h"
#include "../../services.h"
#include "gl.h"
#include "context.h"

#if defined(ARPHEG_PLATFORM_X11)
	#include "../../application/x11window.h"
#endif

template<typename T> inline
T enforce(T value, const char* const error = "Error!") {
    if (!value) services::logging()->critical(error);
    return value;
}

namespace rendering {
namespace opengl {

static bool isExtensionSupported(const char *extList, const char *extension) {
	const char *start;
	const char *where, *terminator;
 
	/* Extension names should not have spaces. */
	where = strchr(extension, ' ');
	if ( where || *extension == '\0' )
	return false;
 
	/* It takes a bit of care to be fool-proof about parsing the
		OpenGL extensions string. Don't be fooled by sub-strings,
		etc. */
	for ( start = extList; ; ) {
	where = strstr( start, extension );
 
	if ( !where )
		break;
 
	terminator = where + strlen( extension );
 
	if ( where == start || *(where - 1) == ' ' )
		if ( *terminator == ' ' || *terminator == '\0' )
		return true;
 
	start = terminator;
	}
 
	return false;
}

#ifndef ARPHEG_RENDERING_GLES
	//Debug output
	static const char* glDebugSourceStr(GLenum src){
		switch(src){
		case GL_DEBUG_SOURCE_API_ARB: return "API";
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return "Shader compiler";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: return "Windowing system";
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: return "Third party";         
		case GL_DEBUG_SOURCE_APPLICATION_ARB: return "App";
		}
		return "";
	}
	static const char* glDebugTypeStr(GLenum type){
		switch(type){
		case GL_DEBUG_TYPE_ERROR_ARB: return "Error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "Deprecated behavior";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return "Undefined behavior";
		case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "Perfomance warning";
		case GL_DEBUG_TYPE_PORTABILITY_ARB: return "Portability warning";
		case GL_DEBUG_TYPE_OTHER_ARB: return "Other";
		}
		return "";
	}
	static void  debugCallback(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar* message,GLvoid* userParam){
		using namespace core::bufferStringStream;
		Formatter fmt;
		printf(fmt.allocator,"GL DEBUG - source '%s' type '%s' severity '%s' message '%s'",
			glDebugSourceStr(source),
			glDebugTypeStr(type),
			severity == GL_DEBUG_SEVERITY_HIGH_ARB? "high" : severity == GL_DEBUG_SEVERITY_MEDIUM_ARB? "med" : "low",
			(const char*)message);

		if(type == GL_DEBUG_TYPE_ERROR_ARB)
			services::logging()->error(asCString(fmt.allocator));
		else services::logging()->warning(asCString(fmt.allocator));
	}
	static void initDebugOutput(Context* context){
		if(context->extensionSupported(extensions::ARB_debug_output)){
			auto glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)context->getProcAddress("glDebugMessageCallbackARB");
			if(glDebugMessageCallbackARB){
				glDebugMessageCallbackARB((GLDEBUGPROCARB)&debugCallback,(const GLvoid*)context);
			}
		}
	}
#endif

static void initContext(Context* context){
#ifndef ARPHEG_RENDERING_GLES
	initDebugOutput(context);
#endif
}

#if  defined(ARPHEG_PLATFORM_MARMALADE)
	Context::Context() {
		IwGLInit(); 
		apiSupport = 0;
		extCheck = extSupport = 0;
		version_ = vec2i(2,0);//GLes 2.0
	}
	Context::~Context() {
		IwGLTerminate();
	}
	vec2i Context::frameBufferSize() {
		return vec2i( IwGLGetInt(IW_GL_WIDTH), IwGLGetInt(IW_GL_HEIGHT) );
	}
	void Context::swapBuffers(bool vsync) {
		IwGLSwapBuffers();
	}
#else

#ifndef PLATFORM_RENDERING_GLES
	void Context::checkApiSupport() {
		using namespace support;
		auto logger = services::logging();

		auto major = version_.x,minor = version_.y;
		apiSupport = 0;
		if(major >= 3){
			if(minor >= 1) {
				if(!glDrawArraysInstanced || !glDrawElementsInstanced){
					logger->warning("OpenGL reports version >= 3.1, but it doesn't provide 'glDrawArraysInstanced' and/or 'glDrawElementsInstanced'");
				} else apiSupport |= GL3_hardware_instancing;
				if(!glUniformBlockBinding){
					logger->warning("OpenGL reports version >= 3.1, but it doesn't provide 'glUniformBlockBinding'");
				} else apiSupport |= GL3_uniform_buffer_objects;
				apiSupport |= GL3_texture_buffer_objects;
			}
			if(minor >= 2) {
				apiSupport |= GL3_geometry_shaders | GL3_ms_textures;
			}
			if(minor >= 3){
				if(!glGenSamplers || !glDeleteSamplers || !glBindSampler){
					logger->warning("OpenGL reports version >= 3.3, but it doesn't provide 'glGen/Delete/BindSamplers'");
				}
				else apiSupport |= GL3_sampler_objects;
			}
		}
		if(major >= 4){
			apiSupport |= GL4_tesselation;
		}
	}
#endif

#if defined(ARPHEG_PLATFORM_WIN32)
	Context::Context() {
		auto target = services::application()->mainWindow();
		assert(target->isCreated());

		int version; //opengl version
	
		HWND hwnd = (HWND)target->handle();
		HDC hdc;
		HGLRC hrc;

		//handle window
		hdc = enforce(GetDC(hwnd));
		PIXELFORMATDESCRIPTOR pfd = {
			sizeof (PIXELFORMATDESCRIPTOR), //  size of this pfd  
			1, // version number  
			PFD_DRAW_TO_WINDOW | // support window  
			PFD_SUPPORT_OPENGL | // support OpenGL  
			PFD_DOUBLEBUFFER, // double buffered  
			PFD_TYPE_RGBA, // RGBA type  
			32, // 24-bit color depth  
			0, 0, 0, 0, 0, 0, // color bits ignored  
			0, // no alpha buffer  
			0, // shift bit ignored  
			0, // no accumulation buffer  
			0, 0, 0, 0, // accum bits ignored  
			0, // 32-bit z-buffer      
			0, // no stencil buffer  
			0, // no auxiliary buffer  
			PFD_MAIN_PLANE, // main layer  
			0, // reserved  
			0, 0, 0 // layer masks ignored  
		};
		enforce(SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd));
		//temporary context
		HGLRC tempContext = enforce(wglCreateContext(hdc), "OpenGL: Failed to create a context!");
		wglMakeCurrent(hdc, tempContext);
		//check version	
		const unsigned char* versionString = enforce(glGetString(GL_VERSION));
		int major = (int) (versionString[0] - '0'), minor = (int) (versionString[2] - '0');
		version = major * 10 + minor;
#ifndef NDEBUG
		bool useDebug = true;
#else
		bool useDebug = false;
#endif
		if (version >= 30) {
			int attribs[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, major,
				WGL_CONTEXT_MINOR_VERSION_ARB, minor,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | (useDebug? WGL_CONTEXT_DEBUG_BIT_ARB : 0),
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
			};
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) enforce(wglGetProcAddress("wglCreateContextAttribsARB"), "Failed to load wglCreateContextAttribsARB!");
			hrc = enforce(wglCreateContextAttribsARB(hdc, 0, attribs), "Failed to create extended OpenGL context!");
			wglMakeCurrent(0, 0);
			wglDeleteContext(tempContext);
		} else {
			wglMakeCurrent(0, 0);
			hrc = tempContext;
		}
		wglMakeCurrent(hdc, hrc);
		data[0] = (void*)hdc;
		data[1] = (void*)hrc;

		loadOpenGLCore(version);
		extCheck = extSupport = 0;
		version_ = vec2i(major,minor);
		vsync_ = false;
		checkApiSupport();
		initContext(this);
	}
	Context::~Context() {
		wglMakeCurrent(0, 0);
		wglDeleteContext((HGLRC)data[1]);
		ReleaseDC((HWND)services::application()->mainWindow()->handle(),(HDC)data[0]);
	}
	static void setVsync(bool vsync){
		//PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = nullptr;
		 PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
		 if(wglSwapIntervalEXT){
			 wglSwapIntervalEXT(vsync? 1 : 0);
		 }
	}
	void Context::swapBuffers(bool vsync) {
		if(vsync != vsync_){
			setVsync(vsync);
			vsync_ = vsync;
		}
		::SwapBuffers((HDC)data[0]);
	}
	void* Context::getProcAddress(const char* name) {
		return wglGetProcAddress(name);
	}
#elif defined(ARPHEG_PLATFORM_X11)

	static bool ctxErrorOccurred = false;
	static int ctxErrorHandler( Display *dpy, XErrorEvent *ev ) {
		ctxErrorOccurred = true;
		return 0;
	}

	Context::Context() {
		auto target = services::application()->mainWindow();
		assert(target->isCreated());

		auto data = (application::X11Window*)target->handle();
		auto logging = services::logging();

		//GLX3
		const char *glxExts = glXQueryExtensionsString( data->display,DefaultScreen( data->display ) );
 
		// NOTE: It is not necessary to create or make current to a context before
		// calling glXGetProcAddressARB
		PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

		GLXContext ctx = 0;
 
		// Install an X error handler so the application won't exit if GL 3.0
		// context allocation fails.
		//
		// Note this error handler is global.  All display connections in all threads
		// of a process use the same error handler, so be sure to guard against other
		// threads issuing X commands while this code is running.
		ctxErrorOccurred = false;
		int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

		// Check for the GLX_ARB_create_context extension string and the function.
		// If either is not present, use GLX 1.3 context creation method.
		if ( !isExtensionSupported( glxExts, "GLX_ARB_create_context" ) || !glXCreateContextAttribsARB ){
			logging->fatalSolution("OpenGL 3+ isn't supported - glXCreateContextAttribsARB() not found!","Please update your graphics card's drivers!");
			ctx = glXCreateNewContext( data->display, data->bestFbc, GLX_RGBA_TYPE, 0, True );
		}
		else {
#ifndef NDEBUG
			bool useDebug = true;
#else
			bool useDebug = false;
#endif
			int context_attribs[] =
			{
			GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 0,
			GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | (useDebug? GLX_CONTEXT_DEBUG_BIT_ARB : 0),
			None
			};
			ctx = glXCreateContextAttribsARB( data->display, data->bestFbc, 0,True, context_attribs );
 
			// Sync to ensure any errors generated are processed.
			XSync( data->display, False );
			if ( !ctxErrorOccurred && ctx ){
				ctxErrorOccurred = false;
			}
			else {
				// Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
				// When a context version below 3.0 is requested, implementations will
				// return the newest context version compatible with OpenGL versions less
				// than version 3.0.
				// GLX_CONTEXT_MAJOR_VERSION_ARB = 1
				context_attribs[1] = 1;
				// GLX_CONTEXT_MINOR_VERSION_ARB = 0
				context_attribs[3] = 0;
 
				ctxErrorOccurred = false;
 
				logging->fatalSolution("OpenGL 3+ isn't supported - Failed to create OpenGL 3 context!","Please update your graphics card's drivers!");
				ctx = glXCreateContextAttribsARB( data->display, data->bestFbc, 0, True, context_attribs );
			}
		}
 
		// Sync to ensure any errors generated are processed.
		XSync( data->display, False );
 
		// Restore the original error handler
		XSetErrorHandler( oldHandler );
 
		if ( ctxErrorOccurred || !ctx ) {
			logging->fatal("Failed to create an OpenGL context");
			return;
		}
 
		// Verifying that context is a direct context
		if ( ! glXIsDirect ( data->display, ctx ) ) {
			logging->warning("Indirect GLX rendering context obtained\n");
		}
 
		glXMakeCurrent( data->display, data->win, ctx );

		data->ctx = ctx;

		//loadOpenGLCore(version);
		extCheck = extSupport = 0;
		//version_ = vec2i(major,minor);
		vsync_ = false;
		checkApiSupport();
		initContext(this);
	}
	Context::~Context() {
		auto data = (application::X11Window*)services::application()->mainWindow()->handle();
		glXMakeCurrent( data->display, 0, 0 );
		glXDestroyContext( data->display, data->ctx );
	}
	static void setVsync(application::X11Window* data,bool vsync){
		 PFNGLXSWAPINTERVALEXTPROC glxSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC) glXGetProcAddressARB((const GLubyte *)"glxSwapIntervalEXT");
		 if(glxSwapIntervalEXT){
			 glxSwapIntervalEXT(data->display,data->win,vsync? 1 : 0);
		 }		
	}
	void Context::swapBuffers(bool vsync) {
		if(vsync != vsync_){
			setVsync((application::X11Window*)services::application()->mainWindow()->handle(),vsync);
			vsync_ = vsync;
		}		
		auto data = (application::X11Window*)services::application()->mainWindow()->handle();
		glXSwapBuffers ( data->display, data->win );
	}
	void* Context::getProcAddress(const char* name) {
		return glXGetProcAddress((const GLubyte*)name);
	}
#endif

	vec2i Context::frameBufferSize() {
		return services::application()->mainWindow()->size();
	}


#endif

static const char* extensionToString(uint32 id){
	using namespace extensions;
#ifdef PLATFORM_RENDERING_GLES
	switch(id) {
	case OES_texture_npot: return "GL_OES_texture_npot";
	case OES_element_index_uint: return "GL_OES_element_index_uint";
	}
#else
	switch(id) {
	case ARB_debug_output: return "GL_ARB_debug_output";
	case AMD_pinned_memory: return "GL_AMD_pinned_memory";
	case ARB_vertex_type_2_10_10_10_rev: return "GL_ARB_vertex_type_2_10_10_10_rev";
	case EXT_direct_state_access: return "GL_EXT_direct_state_access";
	}
#endif
	assert(false && "Invalid extension");
	return nullptr;
}
bool Context::extensionSupported(uint32 extension) {
	if(extCheck & extension){
		return (extSupport & extension) != 0;
	} else {
		extCheck |= extension;
#ifdef PLATFORM_RENDERING_GLES
		auto ext = glGetString(GL_EXTENSIONS);
		bool support = isExtensionSupported((const char*)ext,extensionToString(extension));
#else
		int max = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &max);
		bool support = false;
		auto extstr = extensionToString(extension);
		for(int i = 0; i < max; i++){
			if(isExtensionSupported((const char*)glGetStringi(GL_EXTENSIONS, i),extstr)){
				support = true; break;
			}
		}
#endif

		if( support ){
			core::bufferStringStream::Formatter fmt;
			core::bufferStringStream::printf(fmt.allocator,"GL extension '%s' is supported!",extensionToString(extension));
			services::logging()->information(core::bufferStringStream::asCString(fmt.allocator));
			extSupport |= extension;
			return true;
		}
		return false;
	}
}


} }