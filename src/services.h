#pragma once

#include "core/memory.h"
#include "core/timing.h"
#include "core/logging.h"
#include "application/application.h"
#include "input/input.h"
#include "rendering/rendering.h"

class services {
public:

	static inline application::Service*  application();
	static inline core::timing::Service* timing();
	static inline input::Service*     input();
	static inline rendering::Service* rendering();
	static inline core::logging::Service* logging();
	
	// Frame allocator is also a scratch allocator
	static inline core::Allocator* frameAllocator();
	// The frame allocator will be resized the following frame
	static void setFrameAllocatorSize(size_t bytes);

	static inline core::Allocator* permanentAllocator();

	static void init();
	static void preStep();
	static void postStep();
	static void shutdown();
	static void postWindowCreation();

private:
	static application::Service* app_;
	static core::timing::Service* timing_;
	static input::Service* input_;
	static rendering::Service* rendering_;
	static core::logging::Service* logging_;

	static core::BufferAllocator* frameAllocator_;
	static size_t frameAllocatorSize_;
	static core::Allocator* permanentAllocator_;
};

inline application::Service*  services::application() { return app_; }
inline core::timing::Service* services::timing()   { return timing_; }
inline input::Service*     services::input()       { return input_; }
inline rendering::Service* services::rendering()   { return rendering_; }
inline core::Allocator* services::frameAllocator() { return frameAllocator_; }
inline core::Allocator* services::permanentAllocator() { return permanentAllocator_; }
inline core::logging::Service* services::logging() { return logging_; }
