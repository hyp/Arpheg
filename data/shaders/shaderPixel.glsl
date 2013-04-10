#require core.shaders.lighting.tiled
#require core.shaders.lighting.types
#require core.shaders.lighting.phong

in vec2 uv;
in vec3 vsNormal;
in vec3 vsPos;
in vec3 vsView;

uniform sampler2D texture;
out vec3 fragment;

void main(){
	Light light;
	light.position = -vec4(0.0,0.0,-1.0,8.0);
	light.colour = vec4(1.0,1.0,1.0,0.0);
	light.attenuation = vec4(0.0,0.0,0.0,0.0);
	Light point;
	point.position = vec4(1.0,1.0,1.0,2.0);
	point.colour = vec4(1.0,0.0,0.0,0.0);
	point.attenuation = vec4(0.1,0.0,0.5,0.0);
	vec3 N = normalize(vsNormal);
	vec3 diffuse = pow(texture2D(texture,uv).xyz,vec3(2.2));
	vec3 specular = vec3(0.3);
	fragment = lightingPhongEvalDirectional(light,N,normalize(vsView),diffuse,specular,96.0);
	fragment += lightingPhongEvalPointCone(point,vsPos,N,normalize(vsView),diffuse,specular,96.0);
	
	//apply gamma correction
	fragment = pow(fragment, vec3(1.0/2.2));
	//fragment = normalize(vsView)*0.5 + vec3(0.5);//texture2D(texture,uv).xyz;
}
	