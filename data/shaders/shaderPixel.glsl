
in vec2 uv;
uniform sampler2D texture;
out vec3 fragment;

void main(){
	fragment = texture2D(texture,uv).xyz;
}
	