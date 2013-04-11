// Provides an implementation for the CPU based light tile grid for the tile based forward/deferred renderer.

#pragma once

#include "../types.h"
#include "../dynamicBuffer.h"

namespace rendering {
namespace lighting {

class TileGrid {
public:
	typedef uint32 Tile;
	enum { kIndexBufferMaxSize = 0xFFFFF };//20 bits for index per tile.
	enum { kMaxLightsPerTile = 0xFFF };    //12 bits for count per tile.

	//Max 0xFFFF lights.
	typedef uint16 LightIndex;
	
	struct LightAABB {
		int16 min[2];
		int16 max[2];
	};
	struct Tiler {
		enum { kMaxLightsPerView = 2048 };

		Tiler(const Camera& camera,const Viewport& viewport); 
		void tile(const vec4f& lightSphere,LightIndex lightId);
		

		const mat44f& cameraMatrix;
		vec4f  screenCenter;
		vec2i  viewSizeMinus1;
		uint32 offset;
		LightAABB  lightAABB [kMaxLightsPerView];
		LightIndex lightIndex[kMaxLightsPerView];
	};

	TileGrid();
	~TileGrid();
	void updateViewport(const Viewport& viewport);
	void performLightAssignment(const Tiler& tiler);
	//Propagates the light assignment changes to the gpu.
	void updateBuffers();

	core::Bytes tileBuffer();
	core::Bytes indexBuffer();

	inline TextureBuffer tileBufferTexture()  const;
	inline TextureBuffer indexBufferTexture() const;
	inline uint32 maxLightsPerTile() const;
private:
	void recreate(vec2i size);

	vec2i  size_;
	vec2i  tileSize_;vec2i tileCount_;
	Tile* tiles;
	LightIndex* indexes;
	size_t indexOffset;
	uint32 maxLightsPerTile_;

	DynamicBufferTexture tileBuffer_,indexBuffer_;
};
inline TextureBuffer TileGrid::tileBufferTexture()  const { return tileBuffer_.textureView;  }
inline TextureBuffer TileGrid::indexBufferTexture() const { return indexBuffer_.textureView; }
inline uint32 TileGrid::maxLightsPerTile() const { return maxLightsPerTile_; }

} }