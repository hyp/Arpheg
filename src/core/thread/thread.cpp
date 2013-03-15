#include <limits>
#include "../assert.h"
#include "thread.h"
#include "../platform.h"

namespace core {

Thread::Thread(){
	function_ = nullptr;
	userData_ = nullptr;
	handle_ = nullptr;
}
Thread::Thread(Function function,void* userData){
	handle_ = nullptr;
	start(function,userData);
}
Thread::~Thread(){
	if(handle_){
		join();
	}
}
int Thread::threadProc() {
	assert(function_);
	function_(userData_);
	return 0;
}

#ifdef ARPHEG_PLATFORM_WIN32

static DWORD WINAPI threadProcedure( Thread *thread ) {
    return (DWORD)thread->threadProc();
}
void Thread::start(Function target,void* userData) {
	assert(!handle_);
	function_ = target;
	userData_ = userData;

	auto threadHandle = CreateThread( NULL,0,(LPTHREAD_START_ROUTINE)threadProcedure,this,0,(LPDWORD)&tid_);
	assert(threadHandle);
	handle_ = (void*) threadHandle;
}
void Thread::join(){
	HANDLE threadHandle = (HANDLE)handle_;
    WaitForSingleObject( threadHandle, INFINITE );
    CloseHandle( threadHandle );
	handle_ = nullptr;
}
uint32 Thread::hardwareThreadCount() {
	//http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
	SYSTEM_INFO sysinfo={{0}};
	GetSystemInfo( &sysinfo );
	return (uint32)sysinfo.dwNumberOfProcessors;
}
uint32 Thread::currentID() {
	return (uint32)GetCurrentThreadId();
}
void Thread::yield() {
	SwitchToThread();
}
void Thread::sleep(uint32 ms) {
	Sleep(ms);
}

#elif defined(ARPHEG_PLATFORM_PTHREAD)

void* threadProcedure(void* thread){
	((Thread*) thread)->threadProc();
	return nullptr;
}
void Thread::start(Function target,void* userData) {
	assert(!handle_);
	function_ = target;
	userData_ = userData;

	pthread_create( &thread_, NULL, threadProcedure, (void*) this);
	handle_ = &thread_;
	tid_ = (uint32)thread_;
}
void Thread::join(){
	pthread_join(thread_,NULL);
	handle_ = nullptr;
}
uint32 Thread::hardwareThreadCount() {
	//http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
	return (uint32)sysconf( _SC_NPROCESSORS_ONLN );
}
void Thread::yield() {
	sched_yield();
}
uint32 Thread::currentID() {
	return (uint32)pthread_self();
}
void Thread::sleep(uint32 ms){
	assert(ms < (std::numeric_limits<uint32>::max()/1000));
	usleep(((useconds_t)ms*1000));
}

#else

#endif


}