#include "../core/time.h"
#include "../core/bufferStringStream.h"
#include "../services.h"
#include "profiling.h"

namespace application {
namespace profiling {

struct Result {
	const char* name;
	Timer::TimeType mean;
	Timer::TimeType median;
	Timer::TimeType total;
	void print(){
		core::bufferStringStream::Formatter fmt;
		core::bufferStringStream::printf(fmt.allocator,"%s Profiling results: %f mean, %f median,%f total",name,mean,median,total);
		services::logging()->information(core::bufferStringStream::asCString(fmt.allocator));
	}
};

Timer::Timer(const char* str,uint32 samples) {
	name = str;
	sample = 0;
	sampleCount = samples;
}
void Timer::processResults(){
	Result result;
	result.name = name;
	result.total =  0.0;
	for(uint32 i = 0;i < kNumSamples;i++){
		result.total += time[i];
	}
	result.mean = result.total/TimeType(kNumSamples);
	result.median = time[kNumSamples/2];
	result.print();
}
void Timer::start(){
	clock = core::highResolutionClock::now();
}
void Timer::end(){
	if(services::application()->frameID() < 100) return;//Skip some initial frames.
	if(sample >=kNumSamples){
		if(sample == kNumSamples){
			processResults();
			sample++;
		}
		time[0] = TimeType(core::highResolutionClock::now() - clock);
		return;
	}
	time[sample] = TimeType(core::highResolutionClock::now() - clock);
	sample++;
}

} }