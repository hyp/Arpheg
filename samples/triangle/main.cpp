#include "../../src/arpheg.h"
#include "../../src/services.h"

int main(){
	services::init();
	services::application()->mainWindow()->create("A Triangle example");
	auto renderer = services::rendering();

	vec3f verts[] = {vec3f(-0.5,-0.5,0.0),vec3f(0.0,0.5,0.0),vec3f(0.5,-0.5,0.0)};

	auto vbo = renderer->create(rendering::Buffer::Vertex,false,sizeof(verts),verts);
	rendering::Buffer ibo;ibo.makeNull();
	auto triangle = renderer->create(vbo,ibo,rendering::VertexDescriptor::positionAs3Float());
	
	auto vsh = renderer->create(rendering::Shader::Vertex,""
		"#version 130\n"
		"uniform mat4 mvp;\n"
		"in vec3 position;\n"
		"void main(){\n"
		"	gl_Position = mvp * vec4(position,1.0);\n"
		"}\n"
	);
	auto fsh = renderer->create(rendering::Shader::Pixel,""
		"#version 130\n"
		"out vec3 fragment;\n"
		"void main(){\n"
		"    fragment = vec3(0.2);\n"
		"}\n"
	);
	rendering::Shader shaders[2] = {vsh,fsh};
	rendering::Pipeline::GLSLLayout layout;
	layout.vertexAttributeCount = 1;
	const char* va[] = {"position"};
	layout.vertexAttributes = va;
	auto program = renderer->create(shaders,2,layout);

	rendering::Pipeline::Constant mvp("mvp");

	while(!services::application()->quitRequest()){
		services::preStep();
		renderer->clear(vec4f(0.8,0.8,0.8,1));

		rendering::Viewport viewport;
		viewport.position = vec2i(0,0);
		viewport.size = renderer->context()->frameBufferSize();
		renderer->bind(viewport);
		renderer->bind(program);
		mat44f projection = mat44f::perspective(math::pi/2,float(viewport.size.x)/float(viewport.size.y),0.1,1000.0);
		mat44f view = mat44f::identity();
		renderer->bind(mvp,mat44f::identity());
		renderer->bind(triangle);
		renderer->draw(0,3);

		services::rendering()->context()->swapBuffers();
		services::postStep();
	}
	renderer->release(vbo);
	services::shutdown();
	return 0;
}
