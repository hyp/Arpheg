#pragma once

#include "core/memory.h"
#include "application/application.h"
#include "application/timing.h"
#include "application/logging.h"
#include "application/tasking.h"
#include "input/input.h"
#include "rendering/rendering.h"
#include "data/data.h"
#include "ui/types.h"
#include "scene/types.h"

namespace services {

	inline application::Service*   application();
	inline application::timing::Service*  timing();
	inline application::logging::Service* logging();
	inline application::tasking::Service* tasking();
	inline input::Service*         input();
	inline rendering::Service*     rendering();
	inline rendering::debug::Service* debugRendering();
	inline rendering::animation::Service* animation();
	inline data::Service* data();
	inline ui::Service* ui();
	inline scene::rendering::Service* sceneRendering();

	// Frame allocator is also a scratch allocator
	inline core::BufferAllocator* frameAllocator();
	inline core::Allocator* threadSafeFrameAllocator();
	inline core::BufferAllocator* singleThreadedFrameAllocator();
	// The frame allocator will be resized the following frame
	void setFrameAllocatorSize(size_t bytes);

	inline core::Allocator* permanentAllocator();

	void init();
	void preStep();
	void postStep();
	void shutdown();
	void postWindowCreation();

// private:
	extern application::Service* app_;
	extern application::timing::Service* timing_;
	extern application::tasking::Service* tasking_;
	extern input::Service* input_;
	extern rendering::Service* rendering_;
	extern rendering::debug::Service* debugRendering_;
	extern rendering::animation::Service* animation_;
	extern data::Service* data_;
	extern scene::rendering::Service* sceneRendering_;
	extern ui::Service* ui_;
	extern application::logging::Service* logging_;

	extern core::BufferAllocator* frameAllocator_;
	extern core::Allocator* permanentAllocator_;
};

inline application::Service*  services::application()  { return app_; }
inline application::timing::Service* services::timing()       { return timing_; }
inline application::tasking::Service* services::tasking() { return tasking_; }
inline input::Service*     services::input()           { return input_; }
inline rendering::Service* services::rendering()       { return rendering_; }
inline rendering::debug::Service* services::debugRendering() { return debugRendering_; }
inline rendering::animation::Service* services::animation() { return animation_; }
inline core::BufferAllocator* services::frameAllocator()     { return frameAllocator_; }
inline core::Allocator* services::threadSafeFrameAllocator() { return frameAllocator_; }//TODO
inline core::Allocator* services::permanentAllocator() { return permanentAllocator_; }
inline application::logging::Service* services::logging()     { return logging_; }
inline data::Service* services::data() { return data_;  }
inline scene::rendering::Service* services::sceneRendering() { return sceneRendering_; }
inline ui::Service* services::ui() { return ui_; }
