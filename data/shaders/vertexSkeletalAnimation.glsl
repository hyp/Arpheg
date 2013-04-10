#require core.shaders.constants
#require core.shaders.quaternion
#require core.shaders.entity
#require core.shaders.entity.constantBuffer
#require core.shaders.vertexInput.mesh.skinned
#require core.shaders.view
#require core.shaders.view.constantBuffer

uniform mat4 boneMatrices[ANIMATION_BONES_COUNT];

out vec2 uv;
out vec3 vsNormal;
out vec3 vsPos;
out vec3 vsView;

mat4 calculateBoneAnimation(vec4 weights){
	vec4 boneWeights = fract(weights)*2.0;
	ivec4 boneIds    = ivec4(floor(weights));

	mat4 final = boneMatrices[boneIds.x]*boneWeights.x;
	final += boneMatrices[boneIds.y]*boneWeights.y;
	final += boneMatrices[boneIds.z]*boneWeights.z;
	final += boneMatrices[boneIds.w]*boneWeights.w;
	return final;
}

void main(){
	//Skeletal animation
	mat4 boneTransform = calculateBoneAnimation(weights);
	vec4 localPosition = boneTransform * vec4(position,1.0);
	vec3 localNormal = mat3(boneTransform)*normal;
	
	gl_Position = positionLocalToProjected(entity.transformation,localPosition);
	vsNormal = directionLocalToGlobal(entity.transformation,localNormal);
	vsPos    = positionLocalToGlobal(entity.transformation,localPosition.xyz);
	vsView   = positionGlobalToViewVector(view,vsPos);
	uv = texcoord;
}
