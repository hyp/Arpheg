#pragma once

namespace components {
	struct Timer { 
		float time; 
		inline Timer(): time(0.0f) {}
		inline bool activated() { return time <= 0.0f; }

		static void update(float dt,Timer* begin,Timer* end);
	};
}