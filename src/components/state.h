// A game state abstraction - useful for menus, game screens, pause screens.

#pragma once

#include "../core/memory.h"
#include "../core/allocatorNew.h"

namespace components {
	class State {
	public:
		enum Flags {
			OverlayRendering = 0x1,
			OverlayUpdating  = 0x2,
		};

		inline State(uint32 flags = 0);
		virtual ~State() { }

		virtual void update() {}
		virtual void render() {}

		class Service {
		public:
			Service();
			~Service();

			template<typename T> inline T* enter();
			void leave();
			void returnTo(State* state);

			void update();
			void render();
		private:
			void push(State* state);
			State* head_,*first_;
			size_t headAlloc_;
			size_t stackSize_;
			core::BufferAllocator stateStack_;
		};

	protected:
		friend class Service;
		uint32 flags_;
		State* next_;
	};

	inline State::State(uint32 flags) : flags_(flags){}
	template<typename T> inline T* State::Service::enter(){
		auto state = ALLOCATOR_NEW(&stateStack_,T);
		push(state);
		return state;
	}
}