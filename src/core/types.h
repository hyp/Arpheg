#pragma once

#define PLATFORM_RENDERING_GL 1
#define PLATFORM_RENDERING_GLES 20

//Target platform
//#define ARPHEG_PLATFORM_MARMALADE
#ifndef ARPHEG_PLATFORM_MARMALADE
	#ifdef  _WIN32
		#define ARPHEG_PLATFORM_WIN32 1
	#elif defined(__APPLE__) || defined(MACOSX)
		#define ARPHEG_PLATFORM_OSX 1
		#define ARPHEG_PLATFORM_X11 1
	#else
		#define ARPHEG_PLATFORM_LINUX 1
		#define ARPHEG_PLATFORM_X11 1
	#endif
#endif

//Target CPU architecture
#define ARPHEG_ARCH_ARM
#define ARPHEG_ARCH_X86

#ifdef  ARPHEG_ARCH_X86
	#define ARPHEG_ARCH_SIMD
#endif
#ifndef ARPHEG_ARCH_BIG_ENDIAN
	#define ARPHEG_ARCH_LITTLE_ENDIAN
#endif

//Target renderer
#define ARPHEG_RENDERING_GL 1
//#define ARPHEG_RENDERING_GLES 1

#define ARPHEG_RENDERING_GL_VERSION_MAJOR 2
#define ARPHEG_RENDERING_GL_VERSION_MINOR 0
#define ARPHEG_RENDERING_GL_VERSION 200

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
