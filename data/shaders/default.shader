vertex.layout:
	position: 'float', 3
	normal  : 'float', 3
	texcoord: 'float', 2

glsl.vertex:
	$glsl.version.header
	$vertex.layout
	uniform mat4 mvp;
	out vec2 uv;
	
	void main(){
		gl_Position = mvp * vec4(position,1.0);
		uv = texcoord;
	}

glsl.pixel:
	$glsl.version.header
	
	in vec2 uv;
	uniform sampler2D texture;
	out vec3 fragment;
	
	void main(){
	    fragment = texture2D(texture,uv).xyz;
	}
	