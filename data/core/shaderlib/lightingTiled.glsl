//Implements the tiled base lighting.

uvec2 lightingTiledGetTile(usamplerBuffer texture,float invTileSize,int tilesWidth){
	vec2 pos = gl_FragCoord.xy * invTileSize;
	uint packedTile = texelFetch(texture,int(pos.y) * tilesWidth + int(pos.x)).r;
	//Counter: 20 bits, Offset: 12 bits
	return uvec2(packedTile & 0xFFFFF,packedTile >> 20);
}