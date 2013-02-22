#include "../services.h"
#include "../core/assert.h"
#include "../core/allocatorNew.h"

application::Service*  services::app_ = nullptr;
core::timing::Service* services::timing_ = nullptr;
input::Service*        services::input_ = nullptr;
rendering::Service*    services::rendering_ = nullptr;
core::logging::Service* services::logging_ = nullptr;

core::BufferAllocator* services::frameAllocator_ = nullptr;
size_t services::frameAllocatorSize_ = 0;
core::Allocator* services::permanentAllocator_ = nullptr;

static uint8 byte;
core::BufferAllocator buffer(core::Bytes(&byte,1));

void services::init() {
	buffer.reset(sizeof(core::BufferAllocator) + sizeof(application::Service) + sizeof(core::timing::Service) + sizeof(core::logging::Service) + sizeof(input::Service) + sizeof(rendering::Service) + 128);
	auto allocator = &buffer;
	app_       = ALLOCATOR_NEW(allocator,application::Service);
	timing_    = ALLOCATOR_NEW(allocator,core::timing::Service);
	input_     = ALLOCATOR_NEW(allocator,input::Service);
#ifdef ARPHEG_PLATFORM_MARMALADE
	rendering_ = ALLOCATOR_NEW(allocator,rendering::Service);
#endif

	frameAllocator_ = new(allocator->allocate(sizeof(core::BufferAllocator),alignof(core::BufferAllocator))) 
		core::BufferAllocator(1024*1024);//1MB
	frameAllocatorSize_ = 0;
	permanentAllocator_ = core::memory::globalAllocator();

	logging_ = ALLOCATOR_NEW(allocator,core::logging::Service);
}
void services::postWindowCreation() {
#ifndef ARPHEG_PLATFORM_MARMALADE
	assert(!rendering_);
	rendering_ = ALLOCATOR_NEW(&buffer,rendering::Service);
#endif
}
void services::preStep() {
	frameAllocator_->reset(frameAllocatorSize_);

	application()->servicePreStep();
	input_->servicePreStep();
	rendering_->servicePreStep();
}
void services::postStep() {
	timing()->servicePostStep();
	application()->servicePostStep();
	input()->servicePostStep();
}
void services::shutdown() {
	rendering_->~Service();
	input_->~Service();
	application()->~Service();
	timing()->~Service();
	frameAllocator()->~Allocator();
	logging()->~Service();
}
void services::setFrameAllocatorSize(size_t bytes) {
	frameAllocatorSize_ = bytes;
}

