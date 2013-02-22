#pragma once

#include "../types.h"

struct MersenneTwister {
	inline MersenneTwister();
	inline MersenneTwister(uint32 seed);

	void seed(uint32 seed);
	uint32 random();
private:
	enum { MT_LEN = 624 };

	uint32 mt_buffer[MT_LEN];
	uint32 mt_index;
};

inline MersenneTwister::MersenneTwister() { }
inline MersenneTwister::MersenneTwister(uint32 seed) { this->seed(seed); }