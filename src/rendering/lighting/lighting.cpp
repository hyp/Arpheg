#include <limits>
#include "../../core/assert.h"
#include "../../core/bytes.h"
#include "../../core/memory.h"
#include "../../core/bufferStringStream.h"
#include "../../services.h"
#include "../../application/logging.h"
#include "../../application/tasking.h"
#include "lighting.h"

namespace rendering {
namespace lighting {

Service::Service(size_t maxLightsPerFrame){
	lightsData_ = (Light*)core::memory::globalAllocator()->allocate(sizeof(Light)*maxLightsPerFrame,alignof(Light));
	assertRelease(std::numeric_limits<uint16>::max() >= maxLightsPerFrame);
	lightMax = uint16(maxLightsPerFrame);
	lightCount = 0;
	tileGrids = (TileGrid*) core::memory::align_forward(tileGridStorage_,alignof(TileGrid));
	tileGridCount = 0;
}
Service::~Service(){
}
void Service::servicePreStep(){
	lightCount = 0;
}
void Service::addActiveLight(const Light& light) {
	//TODO Debug: overflow check
	lightsData_[lightCount] = light;
	++lightCount;
}
void Service::createTileGrids(Viewport* viewports,uint32 count) {
	if(count > tileGridCount){
		for(uint32 i = tileGridCount;i<count;++i){
			new(tileGrids + i) TileGrid;
		}
	} else if(count < tileGridCount) {
		for(uint32 i = count;i<tileGridCount;++i){
			tileGrids[i].~TileGrid();
		}
	}
	tileGridCount = count;
	for(uint32 i = 0;i<count;++i){
		tileGrids[i].updateViewport(viewports[i]);
	}
}
void Service::setActiveViewports(Viewport* viewports,uint32 count){
	assert(count <=  kMaxActiveViewports);
	createTileGrids(viewports,count);
}
void Service::transferData() {
	for(uint32 i =0;i<tileGridCount;++i){
		tileGrids[i].updateBuffers();
	}
	struct GPUlight {
		vec4f a,b;
	};
	GPUlight lights[128];
	for(uint32 i = 0;i<lightCount;++i){
		lights[i].a = lightsData_[i].parameterStorage_[0];
		lights[i].a.w = lightsData_[i].parameterStorage_[2].x;
		lights[i].b = lightsData_[i].parameterStorage_[1];
		lights[i].b.w = lightsData_[i].parameterStorage_[2].z;
	}
	lights_.update(core::Bytes(lights,256*sizeof(GPUlight)));
	//tlights_.update(texture::FLOAT_RGBA_32323232,core::Bytes(lightsData_,128*sizeof(Light)));
}

} }
