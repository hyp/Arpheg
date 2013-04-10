struct EntityTransformation {
	mat4 mvp;
	vec4 scaling;
	vec4 rotation;
	vec4 translation;
};

vec4 positionLocalToProjected(const EntityTransformation entity,const vec4 localPosition){
	return entity.mvp * localPosition;
}
vec3 positionLocalToGlobal(const EntityTransformation entity,const vec3 localPosition){
	return quaternionRotate(entity.rotation,localPosition*entity.scaling.xyz) + entity.translation.xyz;
}
vec3 directionLocalToGlobal(const EntityTransformation entity,const vec3 localDirection){
	return quaternionRotate(entity.rotation,localDirection);
}

struct Entity {
	EntityTransformation transformation;
};
