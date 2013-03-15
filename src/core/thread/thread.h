#pragma once

#include "../types.h"

#if defined(ARPHEG_PLATFORM_WIN32)
	#include "../platform.h"
#elif !defined( ARPHEG_PLATFORM_MARMALADE)
	#define ARPHEG_PLATFORM_PTHREAD 1
	#include <pthread.h>
#endif

namespace core {

class Thread {
public:
	typedef void (*Function)(void* userData);

	Thread();
	Thread(Function target,void* userData = nullptr);
	virtual ~Thread();

	void start(Function target,void* userData = nullptr);
	void join();

	inline uint32 id() const;
	inline void* nativeHandle() const;
	inline void* userData() const;

	static uint32 hardwareThreadCount();
	static uint32 currentID();
	static void yield();
	static void sleep(uint32 ms);

	virtual int threadProc();
private:
	Function function_;
	void* userData_;
	void* handle_;
#ifdef ARPHEG_PLATFORM_PTHREAD
	pthread_t thread_;
#endif
	uint32 tid_;

	//Non copyable
	inline Thread(const Thread& other){}
	inline void operator =(const Thread& other) {}
};

inline uint32 Thread::id() const { return tid_; }
inline void*  Thread::userData() const { return userData_; }
inline void*  Thread::nativeHandle() const { return handle_; }

struct Mutex {
	inline Mutex();
	inline ~Mutex();
	inline void lock();
	inline bool tryLock();
	inline void unlock();   
protected:
#ifdef ARPHEG_PLATFORM_WIN32
	CRITICAL_SECTION mutex_;
#elif defined(ARPHEG_PLATFORM_PTHREAD)
	pthread_mutex_t mutex_;
#endif
	friend struct Lock;
	friend struct ConditionVariable;
private:
	//Non copyable
	inline Mutex(const Mutex& other){}
	inline void operator =(const Mutex& other) {}
};
#ifdef ARPHEG_PLATFORM_WIN32
    inline Mutex::Mutex()  { InitializeCriticalSection(&mutex_); }
    inline Mutex::~Mutex() { DeleteCriticalSection(&mutex_); }
    inline void Mutex::lock()   { EnterCriticalSection(&mutex_);  }
    inline bool Mutex::tryLock(){ return TryEnterCriticalSection(&mutex_) == TRUE; }
    inline void Mutex::unlock() { LeaveCriticalSection(&mutex_); }
#elif defined(ARPHEG_PLATFORM_PTHREAD)
    inline Mutex::Mutex()  { mutex_ = PTHREAD_MUTEX_INITIALIZER; }
    inline Mutex::~Mutex() { }
    inline void Mutex::lock()   { pthread_mutex_lock( &mutex_ ); }
    inline bool Mutex::tryLock(){ return pthread_mutex_trylock( &mutex_ ) == 0; }
    inline void Mutex::unlock() { pthread_mutex_unlock( &mutex_ ); }        
#endif

struct Lock {
    inline Lock(Mutex& mutex);
    inline ~Lock();
protected:
#ifdef ARPHEG_PLATFORM_WIN32
    CRITICAL_SECTION mutex_;        
#elif defined(ARPHEG_PLATFORM_PTHREAD)
    pthread_mutex_t mutex_;
#endif
    friend struct ConditionVariable;
};
#ifdef ARPHEG_PLATFORM_WIN32
    inline Lock::Lock(Mutex& mutex) { 
            mutex_ = mutex.mutex_; 
            EnterCriticalSection(&mutex_);
    }
    inline Lock::~Lock()            { LeaveCriticalSection(&mutex_); }
#elif defined(ARPHEG_PLATFORM_PTHREAD)
    inline Lock::Lock(Mutex& mutex) { 
            mutex_ = mutex.mutex_; 
            pthread_mutex_lock( &mutex_ ); 
    }
    inline Lock::~Lock()            { pthread_mutex_unlock( &mutex_ ); }
#endif
 
struct ConditionVariable {
    inline ConditionVariable();
        
    template<typename T>
    inline void wait(T& lock);
    inline void notifyOne();
    inline void notifyAll();
private:
#ifdef ARPHEG_PLATFORM_WIN32
    CONDITION_VARIABLE cv_;
#elif defined(ARPHEG_PLATFORM_PTHREAD)
    pthread_cond_t cv_;
#endif
	//Non copyable
	inline ConditionVariable(const ConditionVariable& other){}
	inline void operator =(const ConditionVariable& other) {}
};
#ifdef ARPHEG_PLATFORM_WIN32
    inline ConditionVariable::ConditionVariable(){ InitializeConditionVariable(&cv_); }
    template<typename T>
    inline void ConditionVariable::wait(T& lock) { SleepConditionVariableCS (&cv_, &lock.mutex_,INFINITE); }
    inline void ConditionVariable::notifyAll()   { WakeAllConditionVariable (&cv_); }
    inline void ConditionVariable::notifyOne()   { WakeConditionVariable (&cv_); }
#elif defined(ARPHEG_PLATFORM_PTHREAD)
    inline ConditionVariable::ConditionVariable(){ cv_ = PTHREAD_COND_INITIALIZER; }
    template<typename T>
    inline void ConditionVariable::wait(T& lock) { pthread_cond_wait( &cv_, &lock.mutex_ ); }
    inline void ConditionVariable::notifyAll()   { pthread_cond_broadcast(&cv_); }
    inline void ConditionVariable::notifyOne()   { pthread_cond_signal(&cv_); }
#endif

}