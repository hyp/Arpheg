in vec3 position;
in vec3 normal;
in vec2 texcoord;

uniform mat4 mvp;
out vec2 uv;

void main(){
	gl_Position = mvp * vec4(position,1.0);
	uv = texcoord;
}
