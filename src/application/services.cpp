#include "../services.h"
#include "../core/assert.h"
#include "../core/allocatorNew.h"

#include "../rendering/debug.h"
#include "../rendering/animation.h"
#include "../ui/ui.h"

application::Service*  services::app_ = nullptr;
application::timing::Service* services::timing_ = nullptr;
application::tasking::Service*  services::tasking_ = nullptr;
input::Service*        services::input_ = nullptr;
rendering::Service*    services::rendering_ = nullptr;
rendering::debug::Service* services::debugRendering_ = nullptr;
rendering::animation::Service* services::animation_ = nullptr;
data::Service* services::data_ = nullptr;
ui::Service* services::ui_ = nullptr;
application::logging::Service* services::logging_ = nullptr;

core::BufferAllocator* services::frameAllocator_ = nullptr;
core::Allocator* services::permanentAllocator_ = nullptr;

namespace services {
	static uint8 bytebb;

	static size_t frameAllocatorSize_ = 0;
	static size_t maxFrameAllocatorSize_ = 1024*1024;//1MB
}
core::BufferAllocator buffer(core::Bytes(&services::bytebb,1));


static void initRenderingServices(core::Allocator* allocator){
	assert(!rendering_);
	services::rendering_ = ALLOCATOR_NEW(allocator,rendering::Service);
	services::debugRendering_ = ALLOCATOR_NEW(allocator,rendering::debug::Service) (allocator);
	services::animation_ = ALLOCATOR_NEW(allocator,rendering::animation::Service) (allocator);
	services::ui_ = ALLOCATOR_NEW(allocator,ui::Service) (allocator);
}

void services::init() {
	buffer.reset(sizeof(core::BufferAllocator) + sizeof(application::Service) + sizeof(data::Service) + sizeof(application::timing::Service) + sizeof(application::logging::Service) + sizeof(input::Service) + sizeof(rendering::Service) + 128
		+ 4096 );
	auto allocator = &buffer;
	logging_   = ALLOCATOR_NEW(allocator,application::logging::Service);
	app_       = ALLOCATOR_NEW(allocator,application::Service);
	timing_    = ALLOCATOR_NEW(allocator,application::timing::Service);
	input_     = ALLOCATOR_NEW(allocator,input::Service);
	tasking_   = ALLOCATOR_NEW(allocator,application::tasking::Service) (allocator);
#ifdef ARPHEG_PLATFORM_MARMALADE
	initRenderingServices(allocator);
#endif
	data_ = ALLOCATOR_NEW(allocator,data::Service) (allocator);

	frameAllocator_ = new(allocator->allocate(sizeof(core::BufferAllocator),alignof(core::BufferAllocator))) core::BufferAllocator(maxFrameAllocatorSize_);//1MB
	frameAllocatorSize_ = 0;
	permanentAllocator_ = core::memory::globalAllocator();
}
void services::postWindowCreation() {
#ifndef ARPHEG_PLATFORM_MARMALADE
	initRenderingServices(&buffer);
#endif
}
void services::preStep() {
	frameAllocator_->reset(frameAllocatorSize_);
	frameAllocatorSize_ = 0;// NB: this is important if frameAllocatorSize_ actually changes during the loop

	tasking_->servicePreStep();
	application()->servicePreStep();
	input_->servicePreStep();
	rendering_->servicePreStep();
	debugRendering_->servicePreStep();
	animation()->servicePreStep();
	ui_->servicePreStep();
}
void services::postStep() {
	ui_->servicePostStep();
	debugRendering_->servicePostStep();
	timing()->servicePostStep();
	application()->servicePostStep();
	input()->servicePostStep();
}
void services::shutdown() {
	ui_->~Service();
	animation_->~Service();
	debugRendering_->~Service();
	rendering_->~Service();
	data_->~Service();
	input_->~Service();
	application()->~Service();
	timing()->~Service();
	frameAllocator()->~BufferAllocator();
	tasking()->~Service();
	logging()->~Service();
}
void services::setFrameAllocatorSize(size_t bytes) {
	frameAllocatorSize_ = bytes;
}