#pragma once

#include "types.h"

#ifdef ARPHEG_PLATFORM_WIN32
	#define ARPHEG_EXPORT __declspec(dllexport)
#else
	#define ARPHEG_EXPORT extern
#endif
