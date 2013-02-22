#include "platform.h"
#include "timing.h"

namespace core {
namespace timing {

#ifdef ARPHEG_PLATFORM_MARMALADE
Service::Service() {
	lastFrameTime = s3eTimerGetMs();
	dt_ = 0.0f;
}
void Service::servicePostStep() {
	uint64 currentFrameTime = s3eTimerGetMs();
	int32 deltaMs = (int32)(currentFrameTime - lastFrameTime);
	if(deltaMs < 0) deltaMs = 1;
	dt_ = float(deltaMs)/1000.0f;
	lastFrameTime = currentFrameTime;
}
#else

#ifdef ARPHEG_PLATFORM_WIN32
double queryTime() {
  __int64 COUNTER;
  __int64 FREQ;
  QueryPerformanceCounter((LARGE_INTEGER*)&COUNTER);
  QueryPerformanceFrequency((LARGE_INTEGER*)&FREQ);
  return static_cast<double>(static_cast<double>(COUNTER) / static_cast<double>(FREQ));
} 
#else
double queryTime(){
	return 0.0;
}
#endif

Service::Service() {
	lastFrameTime = 0;
	lastFrameTimeF = queryTime();
	dt_ = 0.0f;
}
void Service::servicePostStep() {
	auto currentFrameTime = queryTime();
	auto deltaMs = currentFrameTime - lastFrameTimeF;
	if(deltaMs < 0.0f) deltaMs = 0.002f;//2ms
	dt_ = float(deltaMs);
	lastFrameTimeF = currentFrameTime;
}
#endif
Service::~Service(){
}

} }
