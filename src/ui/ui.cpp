#include "../core/allocatorNew.h"
#include "../core/assert.h"
#include "../core/math.h"
#include "../services.h"
#include "../rendering/ui.h"
#include "../rendering/text.h"
#include "../rendering/2d.h"
#include "../input/input.h"
#include "../input/events.h"
#include "ui.h"
#include "widget.h"
#include "events.h"
#include "components.h"

namespace ui {


void Service::updateWidgets() {

	//Handles input for the ui hierarchy
	class InputHandler:public input::events::IHandler {
	public:
		Group* root;
		Widget* focused;

		void onMouseMove(const input::events::Mouse& ev){ 
			events::Mouse evc(ev);
			root->onMouseMove(nullptr,evc);
		}
		void onMouseButton(const input::events::MouseButton& ev){
			events::MouseButton evc(ev);
			root->onMouseButton(nullptr,evc);
		}
		void onMouseWheel(const input::events::MouseWheel& ev){
			if(focused) focused->onMouseWheel(ev);
		}
		void onKey(const input::events::Key& ev) {
			if(focused) focused->onKey(ev);
		}
		void onJoystick(const input::events::Joystick& ev) {
			if(focused) focused->onJoystick(ev);
		}
		void onTouch(const input::events::Touch& ev) {
			events::Touch evc(ev);
			root->onTouch(nullptr,evc);
		}
	};
	InputHandler handler;
	handler.root = root_;
	handler.focused = focused_;
	services::input()->handleEvents(&handler);

	//Handle resize 
	auto newRootSize = services::rendering()->context()->frameBufferSize();
	if(rootSize_.x != newRootSize.x || rootSize_.y != newRootSize.y){
		Widget widget;
		widget.addComponent(root_);
		widget.resize(newRootSize);
		rootSize_ = newRootSize;
	}
}
void Service::drawWidgets() {
	core::BufferAllocator glyphBuffer(sizeof(rendering::text::Glyph)*128,&services::tasking()->threadContext().frameAllocator(),core::BufferAllocator::GrowOnOverflow);	
	
	events::Draw drawEvent;
	drawEvent.layerId = 0;
	drawEvent.renderer = renderer_;
	drawEvent.glyphExtractionBuffer = &glyphBuffer;
	drawEvent.position = vec2i(0,0);
	drawEvent.size = vec2i(rootSize_.x,rootSize_.y);
	Widget widget;
	widget.addComponent(root_);
	widget.draw(drawEvent);

	renderer_->prepareRendering();
}

void Service::setFocus(Widget* widget) {
	focused_ = widget;
}
void Service::render() {
	auto size = services::rendering()->context()->frameBufferSize();
	mat44f matrix = mat44f::ortho(vec2f(0,0),vec2f(float(size.x),float(size.y)));
	renderer_->render(matrix);
}
core::Allocator* Service::componentAllocator() const {
	return core::memory::globalAllocator();
}
void Service::enterLayer() {
	rendering::ui::Batch batch;
	//batch 0 
	batch.name = "innerGeometry";
	batch.depth = 0;
	renderer_->registerBatch(batch,rendering::ui::Service::TexturedColouredPipeline);
	
	//batch 1
	batch.name = "borderGeometry";
	batch.depth = 10;
	renderer_->registerBatch(batch,rendering::ui::Service::TexturedColouredPipeline);
}
void Service::servicePreStep(){
	renderer_->servicePreStep();
	
	enterLayer();
}
void Service::servicePostStep(){
	renderer_->servicePostStep();
}
Service::Service(core::Allocator* allocator){
	renderer_ = ALLOCATOR_NEW(allocator,rendering::ui::Service);
	root_ = ALLOCATOR_NEW(allocator,Group);
	focused_ = nullptr;
	rootSize_ = vec2i(0,0);
}
Service::~Service(){
	renderer_->~Service();
	root_->~Group();
}

}