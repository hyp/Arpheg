#include <string.h>
#include "../core/thread/thread.h"
#include "../core/thread/threadLocal.h"
#include "../core/memory.h"
#include "../core/assert.h"
#include "../core/allocatorNew.h"
#include "../core/bufferStringStream.h"
#include "../services.h"
#include "tasking.h"

//TODO
namespace application {
namespace tasking {

static core::Mutex* myThreadIdCounterLock_;
static uint32 myThreadIdCounter_;
static THREAD_LOCAL(uint32,myThreadId_) = 0xFFFF;


class WorkerThread: public core::Thread {
public:
	Service* service;
	bool done;
	core::Mutex mutex;
	
	WorkerThread(Service* s) : service(s),done(false) {}
	
	void giveTask(Task* task){
		//mutex.lock();
		//this->task = task;
		//mutex.unlock();
		//signal.notifyOne();
	}
	int threadProc(){
		//Aquire custom thread id
		myThreadIdCounterLock_->lock();
		myThreadId_ = myThreadIdCounter_;
		myThreadIdCounter_++;
		myThreadIdCounterLock_->unlock();

wait:
		while(auto task = service->get()){
			task->work(task);
			task->openWorkItems-=1;
		}
		mutex.lock();
		service->workerWaitCondition_->wait(mutex);
		mutex.unlock();
		if(!done)
			goto wait;

		//Mop up
		core::bufferStringStream::Formatter fmt;
		core::bufferStringStream::printf(fmt.allocator,"application.tasking WorkerThread(id:%d) finished",myThreadId_);
		services::logging()->trace(core::bufferStringStream::asCString(fmt.allocator));
		return 0;
	}
};

uint32   Service::threadId() const {
	return myThreadId_;
}
Context& Service::threadContext() const {
	return contexts_[myThreadId_];
}
//256KiB of initial frame data.
Context::Context() : frameAllocator_(1024*256) {
	frameAllocatorReset_ = 0;
}
void Context::resizeFrameAllocator(size_t size) {
	frameAllocatorReset_ = size;
}
void Context::servicePreStep() {
	frameAllocator_.reset(frameAllocatorReset_);
	frameAllocatorReset_ = 0;//NB: reset the reset
}

struct TaskKey {
	TaskID id;
	Task*  task;
	TaskKey* next;
};
struct TaskQueue {

};

struct TaskGroup {
	uint32 current;
	uint32 count;

	TaskGroup() : current(0){
	}
	inline Task* get(){
		if(current >= count) return nullptr;
		auto p = ((Task*)(this+1)) + current;
		current++;
		return p;
	}
	inline Task* last() const {
		return ((Task*)(this+1)) + (count -1);
	}
};
Task* Service::get() {
	mutex_->lock();
	Task* task = nullptr;
	auto group = currentGroup_;
	if(group){
		task = group->get();
		if(!task){
			//Release the group
			currentGroup_ = nullptr;
		}
	}
	mutex_->unlock();
	return task;
}

void Service::servicePreStep(){
	new (taskStorage_) core::BufferAllocator(4096,services::frameAllocator(),core::BufferAllocator::GrowOnOverflow);
	for(uint32 i = 0;i<threadCount_;++i){
		contexts_[i].servicePreStep();
	}
	waitableTasks_ = nullptr;
	currentGroup_ = nullptr;
	taskCounter_ = 0;
}
Service::Service(core::Allocator* allocator,uint32 mainThreads) {
	assertRelease(mainThreads == 1 && "Only one main thread is supported ATM!");
	workerCount_ = core::Thread::hardwareThreadCount() - mainThreads;
	if(!workerCount_) workerCount_ = 1;
	threadCount_ = workerCount_ + mainThreads;

	//Register the current main thread
	myThreadId_ = 0;
	myThreadIdCounter_ = mainThreads;
	myThreadIdCounterLock_ = ALLOCATOR_NEW(allocator,core::Mutex);
	mutex_ = ALLOCATOR_NEW(allocator,core::Mutex);
	workerWaitCondition_ = ALLOCATOR_NEW(allocator,core::ConditionVariable);

	currentGroup_ = nullptr;
	taskStorage_ = (core::BufferAllocator*)allocator->allocate(sizeof(core::BufferAllocator),alignof(core::BufferAllocator));


	//Create thread contexts
	contexts_ = (Context*)allocator->allocate(sizeof(Context)*threadCount_,alignof(Context));
	for(uint32 i = 0;i<threadCount_;++i){
		new(contexts_ + i) Context();
	}
	//Fire up the guns
	workers_ = (WorkerThread*)allocator->allocate(sizeof(WorkerThread)*workerCount_,alignof(WorkerThread));
	for(uint32 i = 0;i<workerCount_;++i){
		new(workers_+i) WorkerThread(this);
		workers_[i].start(nullptr,nullptr);
	}

	core::bufferStringStream::Formatter fmt;
	core::bufferStringStream::printf(fmt.allocator,"application.tasking service has 1 main thread,\n it created %d extra main thread(s), \n and created %d worker thread(s).",mainThreads - 1,workerCount_);
	services::logging()->information(core::bufferStringStream::asCString(fmt.allocator));
	
}
Service::~Service(){
	for(uint32 i = 0;i<workerCount_;++i){
		workers_[i].done = true;
	}
	workerWaitCondition_->notifyAll();
	for(uint32 i = 0;i<workerCount_;++i){
		workers_[i].join();
		workers_[i].~WorkerThread();
	}
	workerCount_ = 0;
	for(uint32 i = 0;i<threadCount_;++i){
		contexts_[i].~Context();
	}
	threadCount_ = 0;
	myThreadIdCounterLock_->~Mutex();
	mutex_->~Mutex();
	workerWaitCondition_->~ConditionVariable();
}

void Service::regWaitableTask(TaskID id,Task* task) {
	auto key = ALLOCATOR_NEW(taskStorage_,TaskKey);
	key->id = id;
	key->task = task;
	key->next = waitableTasks_;
	waitableTasks_ = key;
}
Task* Service::findWaitableTask(TaskID id) {
	for(auto i= waitableTasks_;i!=nullptr;i=i->next){
		if(i->id == id) return i->task;
	}
	return nullptr;
}
TaskID Service::addGroup(Task* tasks,uint32 count,TaskID dependency) {
	//Create a new task group.
	assert(threadId() == 0 && "Tasks can only be added from the main thread");
	assert(count);

	mutex_->lock();
	TaskGroup* group= (TaskGroup*)taskStorage_->allocate(sizeof(TaskGroup)+sizeof(Task)*count,alignof(Task));
	group->current = 0;
	group->count = count;
	Task* dest = (Task*)(group+1);
	auto groupId = taskCounter_;taskCounter_++;
	for(uint32 i = 0;i<count;++i){
		dest[i] = tasks[i];
		dest[i].openWorkItems = 1;//Initialize each task
		dest[i].id = groupId;
	}
	if(!currentGroup_){
		currentGroup_ = group;
		//Wake up the worker threads.
		workerWaitCondition_->notifyAll();
	} else {
		assertRelease(false && "Not implemented");
	}
	mutex_->unlock();
	regWaitableTask(groupId,group->last());
	return groupId;
}
TaskID Service::beginAdd(Task* task,TaskID dependsOn,TaskID parent){
	task->openWorkItems = 2;
	return 0;
}
void Service::endAdd(TaskID task){
	//task->openWorkItems = 1;
}
void Service::add(Task* tasks,uint32 count,TaskID parent){

}
void Service::wait(TaskID taskId){
	assert(threadId() == 0 && "Tasked wait can only be done from the main thread");

	Task* awaitingTask = findWaitableTask(taskId);
	if(awaitingTask){
		while(awaitingTask->openWorkItems){
			//Grab some work while waiting
			while(auto task = get()){
				task->work(task);
				task->openWorkItems-=1;
				if(task == awaitingTask) return;//Lucky
			}
			if(awaitingTask->openWorkItems == 0) break;
			//If no work is available just give up CPU time
			core::Thread::yield();
		}
	}
}

} }