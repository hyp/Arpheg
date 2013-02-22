#include "../../src/arpheg.h"
#include "../../src/services.h"
#include "../../src/resources/resources.h"

int main(){
	services::init();
	services::application()->mainWindow()->create("A Textured Triangle example");
	auto renderer = services::rendering();

	struct Vert { vec3f position;vec2f uv; };
	Vert verts[] = {{vec3f(-0.5,-0.5,0.0),vec2f(0,0)},{vec3f(0.0,0.5,0.0),vec2f(1,0)},{vec3f(0.5,-0.5,0.0),vec2f(1,1)}};

	auto vbo = renderer->create(rendering::Buffer::Vertex,false,sizeof(verts),verts);
	
	auto vsh = renderer->create(rendering::Shader::Vertex,""
		"#version 130\n"
		"uniform mat4 mvp;\n"
		"in vec3 position;\n"
		"in vec2 texcoord;\n"
		"out vec2 uv;\n"
		"void main(){\n"
		"	gl_Position = mvp * vec4(position,1.0);uv = texcoord;\n"
		"}\n"
	);
	auto fsh = renderer->create(rendering::Shader::Pixel,""
		"#version 130\n"
		"in vec2 uv;\n"
		"out vec3 fragment;\n"
		"uniform sampler2D texture;\n"
		"void main(){\n"
		"    fragment = texture2D(texture,uv).xyz;\n"
		"}\n"
	);
	auto program = renderer->create(rendering::VertexDescriptor::positionAs3Float_texcoordAs2Float(),vsh,fsh);

	rendering::Pipeline::Constant mvp("mvp");
	rendering::Pipeline::Constant textureConst("texture");

	auto texture = resources::loadTexture("data/crate.tga");

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
		renderer->bind(texture,0);
		int id = 0;
		renderer->bind(textureConst,&id);
		renderer->bindVertices(vbo);
		renderer->bind(rendering::topology::Triangle);
		renderer->draw(0,3);

		services::rendering()->context()->swapBuffers();
		services::postStep();
	}
	renderer->release(vbo);
	services::shutdown();
	return 0;
}
