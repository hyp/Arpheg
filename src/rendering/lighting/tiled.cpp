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
	kTileSize = 32
};

TileGrid::TileGrid() {
	static_assert (sizeof(Tile) == 4,"");

	tiles = nullptr;
	size_ = vec2i(0,0);
	auto allocator= core::memory::globalAllocator();
	
	//NB: 4MB preallocation.
	indexes = (LightIndex*)allocator->allocate(sizeof(LightIndex)*kIndexBufferMaxSize,alignof(vec4f));
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
	tiles = (Tile*)allocator->allocate(sizeof(Tile)*tileCount_.x*tileCount_.y,alignof(vec4f));

	using namespace core::bufferStringStream;
	Formatter fmt;
	printf(fmt.allocator,"Created buffer for tiled lighting\n  Width: %d Height: %d\n  Width of tile: %d Height of tile: %d",tileCount_.x,tileCount_.y,tileSize_.x,tileSize_.y);
	services::logging()->information(asCString(fmt.allocator));
}
core::Bytes TileGrid::tileBuffer() {
	return core::Bytes(tiles,sizeof(Tile)*size_t(tileCount_.x*tileCount_.y));
}
core::Bytes TileGrid::indexBuffer() {
	return core::Bytes(indexes,indexOffset*sizeof(LightIndex));
}

TileGrid::Tiler::Tiler(const Camera& camera,const Viewport& viewport) : cameraMatrix(camera.projectionView) {
	screenCenter = vec4f(float(viewport.size.x)*0.5f,float(viewport.size.y)*0.5f,1.f,1.f);
	viewSizeMinus1 = viewport.size - vec2i(1,1);
	offset = 0;
}
void TileGrid::Tiler::tile(const vec4f& sphere,LightIndex lightId) {
	vec4f vertices[8];
	vec4f max ,min;

	min = sphere - sphere.wwww();
	max = sphere + sphere.wwww();
	math::utils::gatherBoxVertices(vertices,min.xyz(),max.xyz());

	for(uint32 i = 0;i<8;++i)
		vertices[i] = cameraMatrix*vertices[i];

	//Get 2D AABB in homogenous coords.
	min = max = vertices[0] / vertices[0].wwww();
	for(uint32 i = 1;i<8;++i){
		auto hv = vertices[i] / vertices[i].wwww();
		min = vec4f::min(min,hv);
		max = vec4f::max(max,hv);
	}

	//Homogenous coords => viewport coords.
	min = vec4f::fma(min,screenCenter,screenCenter);
	max = vec4f::fma(max,screenCenter,screenCenter);
	
	lightAABB[offset].min[0] = std::max(int32(min.x),0); lightAABB[offset].min[1] = std::max(int32(min.y),0);
	lightAABB[offset].max[0] = std::min(int32(max.x),viewSizeMinus1.x); lightAABB[offset].max[1] = std::min(int32(max.y),viewSizeMinus1.y);
	lightIndex[offset] = lightId;
	++offset;
}

struct LightAABBXAxis {
	uint8  firstTile,lastTile;
	uint16 index;
};


static inline bool rowIntersect(int32 ty,const TileGrid::LightAABB& lightRect){
	if(ty > lightRect.min[1]){
		if(lightRect.max[1] <= ty) return false;
	}
	else {
		if((ty + kTileSize) <= lightRect.min[1]) return false;
	}
	return true;
}

// This is a naive tile assignment implementation.
void TileGrid::performLightAssignment(const Tiler& tiler) {
	indexOffset = 0;
	uint32 idx = 0;
	size_t lightCount = tiler.offset;
	auto screenSpaceLights = tiler.lightAABB;
	auto indices = tiler.lightIndex;

	assert(tiles);
	assert(lightCount <= std::numeric_limits<LightIndex>::max());

	size_t rowLightCount;
	LightAABBXAxis rowAABB[Tiler::kMaxLightsPerView];
	uint16 rowCounters[256];
	assertRelease(tileCount_.x < 255);

	for(int32 y = 0;y<tileCount_.y;++y){
#ifdef ARPHEG_RENDERING_GL
		auto ty = tileCount_.y*kTileSize - (y+1)*kTileSize;//OpenGL has y 0 at the bottom.
#else
		auto ty = (y)*kTileSize;
#endif
		
		for(int32 x = 0;x<tileCount_.x;++x){
			rowCounters[x] = 0;
		}

		//Find the lights intersecting this row.
		rowLightCount = 0;
		for(size_t i = 0;i< lightCount;++i){
			if(rowIntersect(ty,screenSpaceLights[i])){
				auto firstTile = screenSpaceLights[i].min[0]/kTileSize;
				auto lastTile  = screenSpaceLights[i].max[0]/kTileSize;
				assert(lastTile < tileCount_.x);
				for(int32 x = firstTile;x<=lastTile;++x){
					rowCounters[x]++;
				}
				rowAABB[rowLightCount].firstTile = uint8(firstTile);
				rowAABB[rowLightCount].lastTile = uint8(lastTile);
				rowAABB[rowLightCount].index = indices[i];
				++rowLightCount;
			}
		}

		auto tiles = this->tiles + y*tileCount_.x;
		for(int32 x = 0;x<tileCount_.x;++x){
			auto count = uint32(rowCounters[x]);
			tiles[x] = idx | (count<<20);
			idx+=count;
			rowCounters[x] = 0;
		}
		for(size_t i = 0;i< rowLightCount;i++){
			uint32 lastTile = uint32(rowAABB[i].lastTile);
			LightIndex index = rowAABB[i].index;
			for(uint32 x = rowAABB[i].firstTile;x <= lastTile;++x){
				indexes[(tiles[x]&0xFFFFF) + rowCounters[x]] = index;
				rowCounters[x]++;
			}
		}
	}

	assertRelease(idx <= kIndexBufferMaxSize && "Tiled light index buffer overflowed!");
	indexOffset = idx;
}

void TileGrid::updateBuffers() {
	assert(services::tasking()->isRenderingThread());
	tileBuffer_ .update(texture::UINT_R_32,tileBuffer());
	indexBuffer_.update(texture::UINT_R_16,indexBuffer());
}

} }