#pragma once

#include "../core/types.h"
#include "../core/memory.h"
#include "../core/thread/types.h"

namespace application {
namespace tasking {

typedef uint32 TaskID;
struct Task;
typedef void (* WorkItem)(Task* task);
inline TaskID noDependencies() { return TaskID(0xFFFFFFFF); }

struct Task {
    TaskID id;
    WorkItem work;
    int openWorkItems;
    TaskID parent, dependency;
	void* data;
	uint32 dataU32;
};

// The context for the current thread(main or worker)
// Includes stuff like per frame buffer(linear pointer inc) allocator for each thread.
struct Context {
	inline core::BufferAllocator& frameAllocator();
	//The actual resize will happen when the next frame starts
	void resizeFrameAllocator(size_t size);

	Context();
	void servicePreStep();
private:
	core::BufferAllocator frameAllocator_;
	size_t frameAllocatorReset_;

	//Add Cache padding bytes to avoid false sharing of contexts between cores.
	uint8 cachePadding_[64];
};
inline core::BufferAllocator& Context::frameAllocator() { return frameAllocator_; }

class WorkerThread;
struct TaskGroup;
struct TaskKey;

class Service {
public:
    Service(core::Allocator* allocator,uint32 mainThreads = 1);
    ~Service();
	void servicePreStep();

	TaskID addGroup(Task* tasks,uint32 count,TaskID dependency = noDependencies());

	TaskID beginAdd(Task* task = nullptr,TaskID dependsOn = 0,TaskID parent = 0);
	void endAdd(TaskID task);
	void add(Task* tasks,uint32 count,TaskID parent = 0);
	void wait(TaskID task);

	inline uint32 workerCount() const;
	inline uint32 threadCount() const;

	//The id of the current thread. It is guaranteed to be less than thread count.
	uint32   threadId() const;
	//Thread context is available for main and worker threads
	Context& threadContext() const;
	//Can the current thread be used for rendering?
	inline bool isRenderingThread() const;

	Task* get();
public:
	void regWaitableTask(TaskID id,Task* task);
	Task* findWaitableTask(TaskID id);
	uint32 taskCounter_;
	core::Mutex* mutex_;
	core::ConditionVariable* workerWaitCondition_;
	core::BufferAllocator* taskStorage_;
	TaskKey* waitableTasks_;

	TaskGroup* currentGroup_;
	WorkerThread* workers_;
	uint32 workerCount_;
	uint32 threadCount_;
	Context* contexts_;
	

	inline Task* getTask(TaskID id);
};
inline uint32 Service::workerCount() const { return workerCount_; }
inline uint32 Service::threadCount() const { return threadCount_; }
inline bool Service::isRenderingThread() const { return threadId() == 0; }

} }