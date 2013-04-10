//Phong lighting

vec3 lightingPhongEvalDirectional(Light light,vec3 normal,vec3 viewVector,vec3 materialDiffuse,vec3 materialSpecular,float materialShininess){
	vec3 lightVec = light.position.xyz;
	float diffuseTerm = max(0.0,dot(normal,lightVec));
	vec3 refl = -normalize(reflect(lightVec,normal));
	float specularTerm = pow(max(dot(refl, viewVector), 0.0), materialShininess);
	return (materialDiffuse * diffuseTerm + materialSpecular * specularTerm) * (light.colour.xyz) + materialDiffuse * light.attenuation.xyz;
}
vec3 lightingPhongEvalPointCone(Light light,vec3 position,vec3 normal,vec3 viewVector,vec3 materialDiffuse,vec3 materialSpecular,float materialShininess){
	vec3 diff = light.position.xyz - position;
	float dist = length(diff);
	vec3 lightVec = diff*(1.0/dist);
	float diffuseTerm = max(0.0,dot(normal,lightVec));
	float spotlightTerm = 1.0;
	if(light.colour.w > 0.0){
		spotlightTerm = max(-dot(lightVec, light.direction.xyz), 0.0);
        spotlightTerm = spotlightTerm *  clamp((light.colour.w - spotlightTerm) / (light.colour.w - light.attenuation.w), 0.0, 1.0);
        spotlightTerm = pow(spotlightTerm, light.direction.w);
	}
	vec3 refl = -normalize(reflect(lightVec,normal));
	float specularTerm = pow(max(dot(refl, viewVector), 0.0), materialShininess);
	float attenuationFactor = max(0.0,1.0/(light.attenuation.x+light.attenuation.y*dist+light.attenuation.z*dist*dist));
	
	return (materialDiffuse * diffuseTerm + materialSpecular * specularTerm) * (light.colour.xyz * (spotlightTerm * attenuationFactor));
}
