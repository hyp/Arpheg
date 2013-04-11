//Active lights is the array of lights visible in (possibly influencing) the current frame.

#pragma once

#include "tiled.h"

namespace rendering {
namespace lighting  {

class Service {
public:
	enum { kMaxActiveViewports = 4 };
	enum { kDefaultMaxActiveLights = 2048 };

	Service(size_t maxLightsPerFrame = kDefaultMaxActiveLights);
	~Service();

	void servicePreStep();
	void setActiveViewports(Viewport* viewports,uint32 count);
	void addActiveLight(const Light& light);

	//For Rendering thread only.
	void transferData();

	inline size_t maxActiveLights() const;
	inline Buffer lightBuffer() const;
	inline TileGrid* tileGrid(uint32 i) const;
	inline Light* lights() const;
public:
	Light* lightsData_;
	uint16 lightCount,lightMax;
	DynamicConstantBuffer lights_;
	//DynamicBufferTexture tlights_;
	TileGrid* tileGrids;
	uint32 tileGridCount;
	void createTileGrids(Viewport* viewports,uint32 count);

	uint8 tileGridStorage_[sizeof(TileGrid)*(kMaxActiveViewports+1)];
};

inline Buffer Service::lightBuffer() const { return lights_.buffer; }
inline TileGrid* Service::tileGrid(uint32 i) const { return tileGrids + i; }
inline size_t Service::maxActiveLights() const { return size_t(lightMax); }
inline Light* Service::lights() const { return lightsData_; }

} }