#pragma once

#include "types.h"
#ifdef ARPHEG_PLATFORM_MARMALADE
	#include <s3e.h>
#elif defined(ARPHEG_PLATFORM_WIN32)
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <unistd.h>
#endif
