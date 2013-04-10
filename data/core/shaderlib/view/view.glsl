
struct View {
	mat4 viewMatrix;
	vec4 cameraPosition;
};

vec3 positionGlobalToViewVector(const View view,const vec3 globalPosition){
	return -normalize( (view.viewMatrix * vec4(globalPosition,1.0)).xyz );
}
