#include "../../src/arpheg.h"
#include "../../src/services.h"
#include "../../src/rendering/opengl/gl.h"

int main(){
	services::init();
	services::application()->mainWindow()->create("OpenGL example");
	while(!services::application()->quitRequest()){
		services::preStep();
		if(services::input()->isPressed(input::keyboard::Escape)){
			services::application()->quit();
		}
		glClearColor(1,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);
		services::rendering()->context()->swapBuffers();
		services::postStep();
	}
	services::shutdown();
	return 0;
}