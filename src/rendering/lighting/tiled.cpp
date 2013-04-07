#include <limits>
#include "../../core/assert.h"
#include "../../core/bytes.h"
#include "../../core/memory.h"
#include "../../core/bufferStringStream.h"
#include "../../services.h"
#include "../../application/logging.h"
#include "../../application/tasking.h"
#include "tiled.h"

namespace rendering {
namespace lighting {

enum {
	kTileSize = 8
};

TileGrid::TileGrid() {
	static_assert (sizeof(Tile16) == 4,"");

	tiles = nullptr;
	size_ = vec2i(0,0);
	auto allocator= core::memory::globalAllocator();
	
	indexes = (LightIndex*)allocator->allocate(sizeof(LightIndex)*std::numeric_limits<LightIndex>::max(),alignof(vec4f));
	indexOffset = 0;
}
void TileGrid::updateViewport(const Viewport& viewport) {
	if(size_.x != viewport.size.x || size_.y != viewport.size.y){
		recreate(viewport.size);
	}
}
TileGrid::~TileGrid(){
	auto allocator = core::memory::globalAllocator();
	allocator->deallocate(tiles);
	allocator->deallocate(indexes);
}
void TileGrid::recreate(vec2i size) {
	auto allocator= core::memory::globalAllocator();

	size_ = size;
	tileSize_ = vec2i(kTileSize,kTileSize);
	tileCount_.x = size_.x/tileSize_.x;
	if(size_.x%tileSize_.x) tileCount_.x++;
	tileCount_.y = size_.y/tileSize_.y;
	if(size_.y%tileSize_.y) tileCount_.y++;

	if(tiles) allocator->deallocate(tiles);
	tiles = (Tile16*)allocator->allocate(sizeof(Tile16)*tileCount_.x*tileCount_.y,alignof(vec4f));

	using namespace core::bufferStringStream;
	Formatter fmt;
	printf(fmt.allocator,"Created buffer for tiled lighting\n  Width: %d Height: %d\n  Width of tile: %d Height of tile: %d",tileCount_.x,tileCount_.y,tileSize_.x,tileSize_.y);
	services::logging()->information(asCString(fmt.allocator));
}
core::Bytes TileGrid::tileBuffer() {
	return core::Bytes(tiles,sizeof(Tile16)*size_t(tileCount_.x*tileCount_.y));
}
core::Bytes TileGrid::indexBuffer() {
	return core::Bytes(indexes,indexOffset*sizeof(LightIndex));
}

static inline bool intersects(int32 tx,int32 ty,const TileGrid::LightAABB& lightRect){
	//overlap on the x axis?
	if(tx > lightRect.min[0]){
		if(lightRect.max[0] <= tx) return false;
	}
	else {
		if((tx + kTileSize) <= lightRect.min[0]) return false;
	}
	//overlap on the y axis?
	if(ty > lightRect.min[1]){
		if(lightRect.max[1] <= ty) return false;
	}
	else {
		if((ty + kTileSize) <= lightRect.min[1]) return false;
	}
	return true;
}

// This is a naive tile assignment implementation.
void TileGrid::spawnLightAssignmentTasks(LightAABB* screenSpaceLights,size_t lightCount) {
	indexOffset = 0;
	size_t idx = 0;

	assert(tiles);
	assert(lightCount <= std::numeric_limits<LightIndex>::max());

	for(int32 y = 0;y<tileCount_.y;++y){
		auto ty = y*kTileSize;
		auto tiles = this->tiles + y*tileCount_.x;
	for(int32 x = 0;x<tileCount_.x;++x){
		auto tx = x*kTileSize;

		auto start = idx;
		for(size_t i = 0;i< lightCount;++i){
			if(intersects(tx,ty,screenSpaceLights[i])){
				indexes[idx] = LightIndex(i);
				++idx;
			}
		}
		tiles[tx].offset = Index(start);
		tiles[tx].count  = Index(idx - start);
	} }

	assertRelease(idx <= std::numeric_limits<Index>::max() && "Tiled light index buffer overflowed uint16!");

	indexOffset = idx;
}

void TileGrid::updateBuffers() {
	assert(services::tasking()->isRenderingThread());

	tileBuffer_ .update(texture::UINT_RG_1616,tileBuffer());
	indexBuffer_.update(texture::UINT_R_16,indexBuffer());
}

} }