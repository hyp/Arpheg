#pragma once

#include "tiled.h"

namespace rendering {
namespace lighting  {

class Service {
public:
	enum { kMaxActiveViewports = 4 };

	Service();
	~Service();
	//Rendering thread only.
	void setActiveViewports(Viewport* viewports,uint32 count);

	inline Buffer lights() const;
	inline TileGrid* tileGrid(uint32 i) const;
private:
	DynamicConstantBuffer lights_;
	TileGrid* tileGrids;
	uint32 tileGridCount;
	void createTileGrids(Viewport* viewports,uint32 count);

	uint8 tileGridStorage_[sizeof(TileGrid)*(kMaxActiveViewports+1)];
};

inline Buffer Service::lights() const { return lights_.buffer; }
inline TileGrid* Service::tileGrid(uint32 i) const { return tileGrids + i; }

} }