#require core.shaders.view
#require core.shaders.view.constantBuffer
#require core.shaders.lighting.types
#require core.shaders.lighting.phong
#require core.shaders.lighting.tiled
#require core.shaders.depth.unchanged

in vec2 uv;
in vec3 vsNormal;
in vec3 vsPos;
in vec3 vsView;

uniform sampler2D texture;
out vec3 fragment;

void main(){
	
	//Compute surface properties.
	vec3 N        = normalize(vsNormal);
	vec3 Eye      = normalize(vsView);
	vec3 diffuse  = pow(texture2D(texture,uv).xyz,vec3(2.2));
	vec3 specular = vec3(0.2);
	float shininess = 127.0;
	
	//Apply global lights.
	Light light;
	light.position = -vec4(0.0,0.0,-1.0,8.0);
	light.colour = vec4(1.0,1.0,1.0,0.0);
	light.attenuation = vec4(0.1,0.1,0.1,0.0);
	fragment = vec3(0.0);//lightingBlinnPhongEvalDirectional(light,N,Eye,diffuse,specular,shininess);
	
	//Sum all the tiled lights.
	LIGHTING_TILED_FOREACH_POINTLIGHT(light)
		fragment += lightingBlinnPhongEvalPoint(light,vsPos,N,Eye,diffuse,specular,shininess);
	LIGHTING_TILED_FOREACH_END
	
	//apply gamma correction
	fragment = pow(fragment, vec3(1.0/2.2));
}
	