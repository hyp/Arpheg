// Oren Nayer diffuse lighting.

vec3 lightingOrenNayerEval(const vec3 directionToLight,const vec3 surfaceNormal,const vec3 directionToEye,vec3 materialDiffuse,float materialRoughness){
	float NdotL = dot(surfaceNormal,directionToLight);
	float NdotV = dot(surfaceNormal,directionToEye);
	
	/*
	See: http://fgiesen.wordpress.com/2010/10/21/finish-your-derivations-please/
	float angleVN = acos(NdotV);
	float angleLN = acos(NdotL);
	float alpha = max(angleVN,angleLN);
	float beta = min(angleVN,angleLN);
	float C = sin(alpha) * tan(beta);
	*/
	float C = sqrt((1.0 - NdotV*NdotV) * (1.0 - NdotL*NdotL)) / max(NdotV, NdotL);
	float gamma = dot(directionToEye - surfaceNormal * NdotV,directionToLight - surfaceNormal * NdotL);
	
	float roughnessSquared = materialRoughness*materialRoughness;
	float A = 1.0 - 0.5 * (roughnessSquared / (roughnessSquared + 0.33));
	float B = 0.45 * (roughnessSquared / (roughnessSquared + 0.09));
	float diffuseTerm = max(0.0, NdotL) * (A + B * max(0.0, gamma) * C);
	return materialDiffuse * diffuseTerm;
}