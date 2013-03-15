#include <time.h>
#include "platform.h"
#include "time.h"

namespace core {
namespace highResolutionClock {
#if defined(ARPHEG_PLATFORM_MARMALADE)
	double now(){
		return ((double)s3eTimerGetMs())/1000.0;//Convert to seconds
	}
#elif defined(ARPHEG_PLATFORM_WIN32)
	double now() {
		__int64 COUNTER;
		__int64 FREQ;
		QueryPerformanceCounter((LARGE_INTEGER*)&COUNTER);
		QueryPerformanceFrequency((LARGE_INTEGER*)&FREQ);
		return static_cast<double>(static_cast<double>(COUNTER) / static_cast<double>(FREQ));
	}
#elif defined(ARPHEG_PLATFORM_LINUX)
	double now() {
		timespec res;
		clock_gettime(CLOCK_MONOTONIC,&res);
		double ticks = double(res.tv_sec) + double(res.tv_nsec)/1000000000.0;
		return ticks;
	}
#else
	double now() {
		return 0.0;
	}
#endif
} }
