#include "../core/assert.h"
#include "state.h"

using namespace components;

void testState(){
	State::Service gamestates;
	
	class TestState: public State {
	public:
		bool rendered,updated;

		TestState(uint32 flags = 0) : State(flags),rendered(false),updated(false) {}

		void render(){
			rendered = true;
		}
		void update(){
			updated = true;
		}
	};
	class Gameplay:public TestState {
	};
	class Menu: public TestState {
	public:
		Menu() : TestState(OverlayRendering) {}
	};
	class InnerMenu: public TestState {
	public:
	};

	auto gameplay = gamestates.enter<Gameplay>();
	gamestates.update();
	gamestates.render();
	assert(gameplay->updated);
	assert(gameplay->rendered);
	gameplay->updated = gameplay->rendered = false;

	auto menu = gamestates.enter<Menu>();
	gamestates.update();
	gamestates.render();
	assert(!gameplay->updated);
	assert(gameplay->rendered);
	assert(menu->updated);
	assert(menu->rendered);
	gameplay->updated = gameplay->rendered = false;
	menu->updated = menu->rendered = false;

	auto innerMenu = gamestates.enter<InnerMenu>();
	gamestates.update();
	gamestates.render();
	assert(!gameplay->updated);
	assert(gameplay->rendered);
	assert(!menu->updated);
	assert(!menu->rendered);
	assert(innerMenu->updated);
	assert(innerMenu->rendered);
	gameplay->updated = gameplay->rendered = false;
	menu->updated = menu->rendered = false;

	gamestates.leave();
	gamestates.leave();
	gamestates.update();
	gamestates.render();
	assert(gameplay->updated);
	assert(gameplay->rendered);
	gameplay->updated = gameplay->rendered = false;

	gamestates.enter<Menu>();
	gamestates.enter<InnerMenu>();
	gamestates.returnTo(gameplay);// should be equal to 2 leaves
	gamestates.update();
	gamestates.render();
	assert(gameplay->updated);
	assert(gameplay->rendered);
	gameplay->updated = gameplay->rendered = false;
}

int main(){
	testState();
	return 0;
}