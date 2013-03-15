#include "../services.h"
#include "eventSlots.h"

namespace components {
namespace eventSlots {
	void ActiveKey::update(ActiveKey* begin,size_t count){
		auto end = begin+count;
		auto input = services::input();
	}
	void ActiveRectangle::update(ActiveRectangle* begin,size_t count){
		auto end = begin+count;
		for(auto rect = begin; rect < end;++rect) rect->e = 0;
		auto input = services::input();
		vec2i screenSize = services::rendering()->context()->frameBufferSize();
		for(auto touch = input->touchEvents(), endTouch = input->touchEventsEnd();touch<endTouch;++touch){
			if(!touch->isActive) continue;
			for(auto rect = begin; rect < end;++rect){
				auto rectangle = rect->rectangle.calculate(screenSize);
				if(touch->position.x >= rectangle.x && touch->position.y >= rectangle.y 
					&& touch->position.x <= (rectangle.x + rectangle.z) && touch->position.y <= (rectangle.y + rectangle.w)){
						rect->e |= Touch;
				}
			}
		}
	}
}
}
