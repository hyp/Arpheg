#pragma once

namespace application {
namespace profiling {

class Timer {
public:
	enum { kNumSamples = 100 };
	typedef float TimeType;
	

	
	Timer(const char* str= "",uint32 samples = kNumSamples);
	void processResults();
	void start();
	void end();
	float currentTime();
private:
	const char* name;
	uint32 sample,sampleCount;
	double clock;
	TimeType time[kNumSamples];
};

} }