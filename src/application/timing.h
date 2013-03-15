#pragma once

#include "../core/types.h"

namespace application {
namespace timing {

class Service {
public:
	Service();
	~Service();
	void servicePostStep();

	inline float dt() const;    // Possibly smoothed dt
	inline float rawDt() const; // Raw, unsmoothed dt

#ifdef ARPHEG_PLATFORM_WIN32
	bool win32ThreadAffinity; // = false
#endif

private:
	double lastFrameTime;
	float dt_;
};

inline float Service::dt()    const { return dt_; }
inline float Service::rawDt() const { return dt_; }

} }