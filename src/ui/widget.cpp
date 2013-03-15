#include <string.h>
#include "../core/assert.h"
#include "../services.h"
#include "widget.h"
#include "components.h"
#include "ui.h"

namespace ui {

Widget::Widget(vec2f position,vec2f size) : 
	position_(position),size_(size),state_(StateVisible|StateEnabled),color_(0xFFffFFff),
	next_(nullptr)
{ 
	componentCount_ = 0;	
}

void Widget::show(){
	state_ |= StateVisible;
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i)
		parts[i]->shown();
}
void Widget::hide(){
	state_ &= (~StateVisible);
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i)
		parts[i]->hidden();
}
void Widget::enable(){
	state_ |= StateEnabled;
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i){
		parts[i]->enabled();
	}
}
void Widget::disable(){
	state_ &= (~StateEnabled);
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i){
		parts[i]->disabled();
	}
}
void Widget::onClick(){
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i)
		parts[i]->clicked();
}
void Widget::select(){
	state_ |= StateSelected;
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i){
		parts[i]->selected();
	}
}
void Widget::deselect() {
	state_ &= (~StateSelected);
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i){
		parts[i]->deselected();
	}
}

void* Widget::allocateComponent(size_t sz,size_t align) {
	return services::ui()->componentAllocator()->allocate(sz,align);
}
void Widget::addComponent(Component* component){
	if(componentCount_ < kSmallComponentArray){
		components_[componentCount_] = component;
	} else {
		assert(false && "Need bigger array!"); //TODO
	}
	componentCount_++;
}
void Widget::removeComponent(Component* component){
	auto comps = components();
	for(uint32 i = 0;i<componentCount_;++i){
		if(comps[i] == component){
			for(uint32 j = i+1;j<componentCount_;++j)
				comps[j-1] = comps[j];
			componentCount_--;
			return;
		}
	}
}
void Widget::draw(events::Draw& ev) {
	if((state_ & (StateVisible|StateEnabled)) != (StateVisible|StateEnabled)) return;
	auto pos = ev.position;
	ev.position = ev.position+position_;
	ev.size = size_;
	auto parts = components();
	for(uint32 i =0;i<componentCount_;++i)
		parts[i]->draw(this,ev);
	ev.position = pos;
}

//Dispatch input events to components

#define UI_WIDGET_DISPATCH_INPUT(event,T)\
void Widget::event(T& ev){ \
	if((state_ & (StateVisible|StateEnabled)) == (StateVisible|StateEnabled)){ \
		auto parts = components(); \
		for(uint32 i =0;i<componentCount_;++i) \
			parts[i]->event(this,ev); \
	} }
#define UI_WIDGET_DISPATCH_INPUT_CONST(event,T)\
void Widget::event(const T& ev){ \
	if((state_ & (StateVisible|StateEnabled)) == (StateVisible|StateEnabled)){ \
		auto parts = components(); \
		for(uint32 i =0;i<componentCount_;++i) \
			parts[i]->event(this,ev); \
	} }

void Widget::onMouseMove(events::Mouse& ev) {
	state_ &= ~(StateHover);
	if((state_ & (StateVisible|StateEnabled)) != (StateVisible|StateEnabled)) return;
	auto newPos = ev.position-position_;
	if(!events::contained(newPos,size_)) return;
	auto oldPos = ev.position;
	ev.position = newPos;
	state_ |= StateHover;
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i) 
		parts[i]->onMouseMove(this,ev); 
	ev.position = oldPos;
}

void Widget::onMouseButton(events::MouseButton& ev) {
	if((state_ & (StateVisible|StateEnabled)) != (StateVisible|StateEnabled)) return;
	auto newPos = ev.position-position_;
	if(!events::contained(newPos,size_)) return;
	auto oldPos = ev.position;
	ev.position = newPos;
	state_ |= StateHover;
	auto parts = components(); 
	for(uint32 i =0;i<componentCount_;++i) 
		parts[i]->onMouseButton(this,ev); 
	ev.position = oldPos;
}

UI_WIDGET_DISPATCH_INPUT_CONST(onMouseWheel,events::MouseWheel)
UI_WIDGET_DISPATCH_INPUT_CONST(onKey,events::Key)
UI_WIDGET_DISPATCH_INPUT_CONST(onJoystick,events::Joystick)
UI_WIDGET_DISPATCH_INPUT(onTouch,events::Touch)

#undef UI_WIDGET_DISPATCH_INPUT
#undef UI_WIDGET_DISPATCH_INPUT_CONST


}