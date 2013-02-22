#pragma once

#include "types.h"

namespace core {
namespace timing {

class Service {
public:
	Service();
	~Service();
	void servicePostStep();
	inline float dt() const;
private:
	uint64 lastFrameTime;
	double lastFrameTimeF;
	float dt_;
};

inline float Service::dt() const { return dt_; }

} }