#pragma once

#include "types.h"

#if !defined( ARPHEG_PLATFORM_MARMALADE) && !defined(ARPHEG_PLATFORM_WIN32)
	#define ARPHEG_PLATFORM_PTHREAD 1
	#include <pthread.h>
#endif

namespace core {

class Thread {
public:
	typedef void (*Function)(void* userData);

	Thread();
	Thread(Function target,void* userData = nullptr);
	~Thread();

	void start(Function target,void* userData = nullptr);
	void join();

	inline uint32 id() const;
	void* nativeHandle();
	inline void* userData() const;

	static uint32 hardwareThreadCount();
	static uint32 currentID();
	static void yield();
	static void sleep(uint32 ms);

	int threadProc();
private:
	inline Thread(const Thread& other){}
	inline void operator =(const Thread& other) {}

	Function function_;
	void* userData_;
	void* handle_;
	uint32 tid_;
#ifdef ARPHEG_PLATFORM_PTHREAD
	pthread_t thread_;
#endif
};

inline uint32 Thread::id() const { return tid_; }
inline void* Thread::userData() const { return userData_; }

}