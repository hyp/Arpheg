#require core.shaders.lighting.tiled
#require core.shaders.lighting.types
#require core.shaders.lighting.phong

in vec2 uv;
in vec3 vsNormal;
in vec3 vsPos;

uniform sampler2D texture;
out vec3 fragment;

void main(){
	Light light;
	light.position = -vec4(0.0,0.0,-1.0,8.0);
	light.colour = vec4(1.0,1.0,1.0,0.0);
	light.attenuation = vec4(0.1,0.1,0.1,0.0);
	vec3 N = normalize(vsNormal);
	vec3 diffuse = texture2D(texture,uv).xyz;
	vec3 specular = vec3(0.0);
	fragment = lightingPhongEvalDirectional(light,N,vec3(0.0),diffuse,specular,0.0);
	//fragment = normalize(vsNormal);//texture2D(texture,uv).xyz;
}
	