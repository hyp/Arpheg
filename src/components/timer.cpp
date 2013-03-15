#include "timer.h"

namespace components {
	void Timer::update(float dt,Timer* begin,Timer* end) {
		for(auto timer = begin;timer < end;++timer){
			if(timer->time > 0.0f)
				timer->time -= dt;
		}
	}
}
