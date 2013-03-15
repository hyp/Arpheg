#pragma once

// Target platform
#ifndef ARPHEG_PLATFORM_MARMALADE
	#if defined(_WIN32) || defined(WIN32)
		#define ARPHEG_PLATFORM_WIN32 1
	#elif defined(__APPLE__) || defined(MACOSX)
		#define ARPHEG_PLATFORM_OSX 1
		#define ARPHEG_PLATFORM_X11 1
	#else
		#define ARPHEG_PLATFORM_LINUX 1
		#define ARPHEG_PLATFORM_X11 1
	#endif
#else
	#define ARPHEG_PLATFORM_MOBILE 1
#endif

// Target CPU architecture
#define ARPHEG_ARCH_ARM 1
#define ARPHEG_ARCH_X86 1

#ifdef  ARPHEG_ARCH_X86 
	#define ARPHEG_ARCH_SIMD 1
	#define ARPHEG_ARCH_X86_SSE2 1
#endif
#ifndef ARPHEG_ARCH_BIG_ENDIAN
	#define ARPHEG_ARCH_LITTLE_ENDIAN 1
#endif

//Target renderer
#define ARPHEG_RENDERING_GL 1

#if ARPHEG_PLATFORM_MOBILE 
	#ifndef ARPHEG_RENDERING_GL
		#define ARPHEG_RENDERING_GL 1
	#endif
	#define ARPHEG_RENDERING_GLES 1

	#define ARPHEG_RENDERING_GL_VERSION_MAJOR 2
	#define ARPHEG_RENDERING_GL_VERSION_MINOR 0
	#define ARPHEG_RENDERING_GL_VERSION 200
#endif

#include <stddef.h>

#ifdef ARPHEG_PLATFORM_MARMALADE
	#include <s3eTypes.h>
	//Still using old gcc..
	#ifndef _MSC_VER
		#define nullptr (0)
	#endif
#else
	#include <stdint.h>
	typedef signed   char int8;
	typedef unsigned char uint8;
	typedef signed   short int16;
	typedef unsigned short uint16;
	typedef signed   int int32;
	typedef unsigned int uint32;
	typedef uint64_t uint64;
	typedef int64_t int64;
#endif

#ifndef alignof
        #define alignof(x) __alignof(x)
#endif
