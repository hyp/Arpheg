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

Service::Service(){
	tileGrids = (TileGrid*) core::memory::align_forward(tileGridStorage_,alignof(TileGrid));
	tileGridCount = 0;
}
Service::~Service(){
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
	assert(services::tasking()->isRenderingThread());
	createTileGrids(viewports,count);
}

} }
