#require core.shaders.quaternion
#require core.shaders.entity
#require core.shaders.entity.constantBuffer
#require core.shaders.vertexInput.mesh.static
#require core.shaders.view
#require core.shaders.view.constantBuffer

out vec2 uv;
out vec3 vsPos;
out vec3 vsNormal;
out vec3 vsView;

void main(){
	gl_Position = positionLocalToProjected(entity.transformation,vec4(position,1.0));
	vsPos    = positionLocalToGlobal(entity.transformation,position);
	vsView   = positionGlobalToViewVector(view,vsPos);
	vsNormal = directionLocalToGlobal(entity.transformation,normal);
	uv = texcoord;
}
