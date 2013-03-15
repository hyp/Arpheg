#include "../../src/arpheg.h"
#include "../../src/services.h"

int main(){
	services::init();
	services::logging()->priority(application::logging::Trace);
	services::application()->mainWindow()->create("Arpheg application example");

	while(!services::application()->quitRequest()){
		services::preStep();

		//Check input
		if(services::input()->isPressed(input::keyboard::Escape)){
			services::application()->quit();
		}
		//Get Frame DT
		auto dt = services::timing()->dt();
		//Render
		services::rendering()->clear(vec4f(1,0,0,1));
		services::rendering()->context()->swapBuffers();

		services::postStep();
	}
	services::shutdown();
	return 0;
}