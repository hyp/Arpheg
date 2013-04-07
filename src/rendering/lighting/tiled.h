#pragma once

#include "../types.h"
#include "../dynamicBuffer.h"

namespace rendering {
namespace lighting {

class TileGrid {
public:
	typedef uint16 LightIndex;
	typedef uint16 Index;

	struct Tile16 {
		uint16 offset;
		uint16 count;
	};
	struct LightAABB {
		int16 min[2];
		int16 max[2];
	};

	TileGrid();
	~TileGrid();
	void updateViewport(const Viewport& viewport);
	void spawnLightAssignmentTasks(LightAABB* lightScreenSpaceVertices,size_t count);
	//Propagates the light assignment changes to the gpu.
	void updateBuffers();

	core::Bytes tileBuffer();
	core::Bytes indexBuffer();

	inline TextureBuffer tileBufferTexture()  const;
	inline TextureBuffer indexBufferTexture() const;
private:
	void recreate(vec2i size);

	vec2i  size_;
	vec2i  tileSize_;vec2i tileCount_;
	Tile16* tiles;
	LightIndex* indexes;
	size_t indexOffset;

	DynamicBufferTexture tileBuffer_,indexBuffer_;
};
inline TextureBuffer TileGrid::tileBufferTexture()  const { return tileBuffer_.textureView;  }
inline TextureBuffer TileGrid::indexBufferTexture() const { return indexBuffer_.textureView; }

} }