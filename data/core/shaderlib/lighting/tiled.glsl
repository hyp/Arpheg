//Implements the tiled base lighting.

uniform usamplerBuffer tiledLightingTextures[2];
layout(std140) uniform lights {
	vec4 lights[512*2];
} ;

uvec2 lightingTiledGetTile(usamplerBuffer texture,float invTileSize,int tilesWidth){
	vec2 pos = gl_FragCoord.xy * invTileSize;
	uint packedTile = texelFetch(texture,int(pos.y) * tilesWidth + int(pos.x)).r;
	//Offset: 20 bits, count: 12 bits
	return uvec2(packedTile & 0xFFFFF,packedTile >> 20);
}

#define LIGHTING_TILED_FOREACH_POINTLIGHT(light) \
	uvec2 tile = lightingTiledGetTile(tiledLightingTextures[0],view.oneOverTileSize,view.tilesWidth); \
	for(uint end = tile.x + tile.y;tile.x<end;tile.x++){ \
		int lightId = 2*int(texelFetch(tiledLightingTextures[1],int(tile.x)).r); \
		vec4 a = lights[lightId];\
		vec4 b = lights[lightId+1];\
		Light light = Light(a,vec4(b.xyz,0.0),vec4(a.w,0.0,b.w,0.0),vec4(0.0)); 
		
#define LIGHTING_TILED_FOREACH_END }
