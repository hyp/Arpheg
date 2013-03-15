#include <limits>
#include "../core/assert.h"
#include "state.h"

namespace components {

State::Service::Service():
stateStack_(1024,nullptr,core::BufferAllocator::GrowOnOverflow) {
	head_ = nullptr;
	first_ = nullptr;
	headAlloc_ = 0;
	stackSize_ = 0;
}
State::Service::~Service() {
	while(head_) leave();
	head_ = nullptr;
	first_ = nullptr;
}

struct StateContext {
	State* previous;
	size_t headAlloc;
};

void State::Service::update() {
	if(!head_) return;
	State* prev = nullptr;
	for(auto i = first_;; i = i->next_){
		if(i->flags_ & OverlayUpdating){
			if(prev) prev->update();
		}
		if(i == head_) break;
		prev = i;
	}
	head_->update();
}
void State::Service::render() {
	if(!head_) return;
	State* prev = nullptr;
	for(auto i = first_;; i = i->next_){
		if(i->flags_ & OverlayRendering){
			if(prev) prev->render();
		}
		if(i == head_) break;
		prev = i;
	}
	head_->render();
}
void State::Service::push(State* state) {
	//Allocate state support structure
	auto ctx = ALLOCATOR_NEW(&stateStack_,StateContext);
	ctx->previous  = head_;
	ctx->headAlloc = headAlloc_;
	
	state->next_ = nullptr;

	//Set the new head
	if(!head_) first_ = state;
	else head_->next_ = state;
	head_ = state;

	headAlloc_ = stateStack_.size()-stackSize_;
	stackSize_ = stateStack_.size();
}
void State::Service::leave(){
	assert(head_);

	head_->~State();//NB: Call state destructor

	auto headContext = ((StateContext*)stateStack_.bufferTop()) - 1;
	
	size_t dealloc = headAlloc_;
	head_ = headContext->previous;
	headAlloc_   = headContext->headAlloc;
	
	stateStack_.deallocateFromTop(dealloc);
	stackSize_ = stateStack_.size();
}
void State::Service::returnTo(State* state) {
	while(head_ && head_ != state) leave();
}

}