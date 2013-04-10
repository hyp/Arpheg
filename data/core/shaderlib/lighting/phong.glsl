//Blinn Phong lighting

float fresnelReflectance( vec3 H, vec3 V, float F0 ) {
  float base = 1.0 - dot( V, H );
  float exponential = pow( base, 5.0 );
  return mix(F0,1.0,exponential);
  //return exponential + F0 * ( 1.0 - exponential );
}

vec3 lightingLambertEval(const vec3 directionToLight,const vec3 surfaceNormal,const vec3 materialDiffuse){
	return materialDiffuse*max(dot(surfaceNormal,directionToLight),0.0);
}

//Specular
vec3 lightingBlinnPhongEval(const vec3 directionToLight,const vec3 surfaceNormal,const vec3 directionToEye,const vec3 materialSpecular,const float materialShininess){
	vec3 halfVector = normalize(directionToLight + directionToEye);
	float specularTerm = pow( max(dot(surfaceNormal,halfVector),0.0) , materialShininess);
	return materialSpecular * specularTerm;
}
vec3 lightingBlinnPhongFresnelEval(const vec3 directionToLight,const vec3 surfaceNormal,const vec3 directionToEye,const vec3 materialSpecular,const float materialShininess){
	vec3 halfVector = normalize(directionToLight + directionToEye);
	float specularTerm = pow( max(dot(surfaceNormal,halfVector),0.0) , materialShininess);
    float fresnelTerm = pow(1.0 - max(0.0, dot(directionToEye,halfVector)), 5.0);
	return mix(materialSpecular, vec3(1.0), fresnelTerm) * specularTerm;
}

#define LIGHT_DIRECTIONAL(dest,light,surfacePosition,materialDiffuse,diffuse,specular)\
	vec3 directionToLight = light.position.xyz;   \
	vec3 diffuseReflection = diffuse;\
	vec3 specularReflection = specular;\
	dest = (diffuseReflection + specularReflection) * (light.colour.xyz) + materialDiffuse * light.attenuation.xyz;

#define LIGHT_POINTCONE(dest,light,surfacePosition,materialDiffuse,diffuse,specular)\
	vec3 diff = light.position.xyz - surfacePosition; \
	float dist = length(diff); \
	vec3 directionToLight = diff*(1.0/dist); \
	vec3 diffuseReflection = diffuse; \
	vec3 specularReflection = specular; \
	float attenuationFactor = max(0.0,1.0/(light.attenuation.x+light.attenuation.y*dist+light.attenuation.z*dist*dist)); \
	float spotlightTerm = 1.0; \
	if(light.colour.w > 0.0){ \
		spotlightTerm = max(-dot(directionToLight, light.direction.xyz), 0.0); \
        spotlightTerm = spotlightTerm *  clamp((light.colour.w - spotlightTerm) / (light.colour.w - light.attenuation.w), 0.0, 1.0); \
        spotlightTerm = pow(spotlightTerm, light.direction.w); \
	}	\
	dest = (diffuseReflection + specularReflection) * (light.colour.xyz * (spotlightTerm * attenuationFactor));
	
vec3 lightingPhongEvalDirectional(const Light light,const vec3 surfaceNormal,const vec3 directionToEye,const vec3 materialDiffuse,const vec3 materialSpecular,const float materialShininess){
	vec3 dest;
	LIGHT_DIRECTIONAL(dest,light,surfaceNormal,materialDiffuse,
	lightingLambertEval(directionToLight,surfaceNormal,materialDiffuse),
	lightingBlinnPhongFresnelEval(directionToLight,surfaceNormal,directionToEye,materialSpecular,materialShininess));
	return dest;
}
vec3 lightingPhongEvalPointCone(const Light light,const vec3 surfacePosition,const vec3 surfaceNormal,const vec3 directionToEye,const vec3 materialDiffuse,const vec3 materialSpecular,const float materialShininess){
	vec3 dest;
	LIGHT_POINTCONE(dest,light,surfacePosition,materialDiffuse,
	lightingLambertEval(directionToLight,surfaceNormal,materialDiffuse),
	lightingBlinnPhongFresnelEval(directionToLight,surfaceNormal,directionToEye,materialSpecular,materialShininess));
	return dest;
}
