#include "../core/platform.h"
#include "../core/time.h"

#include "timing.h"

namespace application {
namespace timing {

Service::Service() {
	lastFrameTime = core::highResolutionClock::now();
	dt_ = 0.0f;

#ifdef ARPHEG_PLATFORM_WIN32
	win32ThreadAffinity = false;
#endif
}
void Service::servicePostStep() {

#ifdef ARPHEG_PLATFORM_WIN32
	DWORD_PTR oldmask;
	HANDLE currentThread;
	if(win32ThreadAffinity){
		currentThread = ::GetCurrentThread();
		oldmask = ::SetThreadAffinityMask(currentThread, 0);
	}
#endif

	auto currentFrameTime = core::highResolutionClock::now();
	auto deltaMs = currentFrameTime - lastFrameTime;
	if(deltaMs < 0.0f) deltaMs = 0.002f;//2ms
	dt_ = float(deltaMs);
	lastFrameTime = currentFrameTime;

#ifdef ARPHEG_PLATFORM_WIN32
	if(win32ThreadAffinity) ::SetThreadAffinityMask(currentThread, oldmask);
#endif

}
Service::~Service(){
}

} }
