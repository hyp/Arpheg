#include <stdio.h>
#include <string.h>
#include "../rootPath.h"
#include "../../src/arpheg.h"
#include "../../src/services.h"
#include "../../src/data/data.h"

#include "../../src/rendering/geometryBatcher.h"
#include "../../src/rendering/text.h"
#include "../../src/rendering/debug.h"
#include "../../src/ui/ui.h"
#include "../../src/ui/components.h"
#include "../../src/ui/widget.h"
#include "../../src/rendering/softwareOcclusion/softwareOcclusion.h"
#include "../../src/rendering/animation.h"
#include "../../src/application/profiling.h"

int main(){
	services::init();
	services::logging()->priority(application::logging::Trace);
	services::application()->mainWindow()->create("Arpheg resources example");

	auto renderer = services::rendering();
	auto data     = services::data();
	
	rendering::Pipeline::Constant mvp("mvp");
	rendering::Pipeline::Constant textureConst("texture");

	//Data
	data->loadBundle(ARPHEG_ROOT_PATH "data/default/default.txt","default");
	auto debugPipeline = data->pipeline("arpheg.debug.pipeline");

	data->loadBundle(ARPHEG_ROOT_PATH "data/default/ui.txt","ui");
	services::ui()->loadData("ui");
	auto sprite = data->sprite("icon");
	ui::Image image(sprite);
	ui::Widget imageWidget(vec2f(0,0),vec2f(renderer->context()->frameBufferSize().x,renderer->context()->frameBufferSize().y));
	imageWidget.addComponent(&image);
	ui::Clickable button;
	imageWidget.addComponent(&button);
	services::ui()->root()->addChild(&imageWidget);
	class This { public:
		void foo(){
			services::application()->quit();
		}
		This(ui::Widget& widget){
			ARPHEG_UI_CONNECT(widget,clicked(),this,foo());
		}
	};
	This s(imageWidget);

	

	data->loadBundle(ARPHEG_ROOT_PATH "data/sample.txt","data");	
	auto mesh    = data->mesh("head");
	auto animation = data->animation("head.animation.0");
	auto program = data->pipeline("default");
	auto texture = data->texture2D("crate");
	auto font    = data->font("font");


	ui::Text text("Hello",font,0xFFFFFFFF,0xFF0000FF);
	ui::Widget textWidget(vec2f(32,402),vec2f(100,400));
	textWidget.addComponent(&text);
	services::ui()->root()->addChild(&textWidget);

	rendering::softwareOcclusion::DepthBuffer depthBuffer(vec2i(1920/8,1080/6));


	application::profiling::Timer profDepthClear("Clear depth");
	application::profiling::Timer profBinBoxes("Bin Boxes");
	application::profiling::Timer profRasterizeTiles("Rasterize Tiles");
	application::profiling::Timer profQuery("Query occlusion");

	float cam = 60.f;
	float anim = 0.f;

	while(!services::application()->quitRequest()){
		services::preStep();
		services::ui()->updateWidgets();

		renderer->clear(vec4f(0.8,0.8,0.8,1));
		
		if(services::input()->isPressed(input::keyboard::Up)){
			cam-=1.6f*services::timing()->dt();
		}
		if(services::input()->isPressed(input::keyboard::Down)){
			cam+=1.6f*services::timing()->dt();
		}

		rendering::Viewport viewport;
		viewport.position = vec2i(0,0);
		viewport.size     = renderer->context()->frameBufferSize();
		renderer->bind(viewport);
		renderer->bind(program->pipeline());
		mat44f projection = mat44f::perspective(math::pi/2,float(viewport.size.x)/float(viewport.size.y),0.1,1000.0);
		mat44f view       = mat44f::lookAt     (vec3f(cam,cam,cam),vec3f(0,0,0),vec3f(0,1,0));
		auto pv= projection*view;
		renderer->bind(mvp,pv);
		renderer->bind(texture,0);
		int id = 0;

		struct Box {
			vec3f min,max;
		};
		Box boxes[] = {{vec3f(0,1.0f,1.0f),vec3f(1,1.8f,1.6f)},{vec3f(1.0,0.5f,0.0f),vec3f(1.4f,1.9f,0.8f)}};
		auto boxCount = sizeof(boxes)/sizeof(boxes[0]);

		uint32 cov = 0;
		bool vis = true;
		depthBuffer.setCamera(pv,0.1f,1000.0f,math::pi/2,vec3f(cam,cam,cam),(vec3f(0,0,0)-vec3f(cam,cam,cam)).normalize(),vec3f(0,1,0));
		profDepthClear.start();
		depthBuffer.clear(1.0f);
		profDepthClear.end();
		profBinBoxes.start();
		for(uint32 i = 0;i<100/boxCount;++i){
			for(uint32 j = 0;j<boxCount;++j){
				depthBuffer.binAABB(boxes[j].min,boxes[j].max);
			}
		}
		profBinBoxes.end();
		profRasterizeTiles.start();
		//cov = depthBuffer.rasterizeTiles();
		services::tasking()->wait(depthBuffer.createRasterizationTasks());
		profRasterizeTiles.end();
		profQuery.start();
		for(uint32 i = 0;i<100;++i) 
			vis = depthBuffer.testAABB(vec3f(0.5,1.5,1.5),vec3f(0.75,1.75,1.75));
		profQuery.end();



		//vis = depthBuffer.testAABB(vec3f(0.5,1.5,1.5),vec3f(0.75,1.75,1.75));
		auto animator = services::animation();
		auto nodes = animator->transformationBuffer().allocate(mesh->skeletonNodeCount());
		anim+=services::timing()->dt();
		rendering::animation::Animator::animate(mesh,animation,anim,nodes);
		
		rendering::Pipeline::Constant c("boneMatrices");
		for(size_t i = 0;i<mesh->submeshCount();++i){
			auto submesh = mesh->submesh(i);

			auto bones = animator->transformationBuffer().allocate(mesh->skeletonNodeCount());
			rendering::animation::Animator::bindSkeleton(submesh,mesh->skeletonNodeCount(),nodes,bones);
			renderer->bind(c,(void*)bones);
			
			renderer->bind(submesh->mesh(),submesh->primitiveKind(),submesh->indexSize());
			renderer->drawIndexed(submesh->primitiveOffset(),submesh->primitiveCount());
		}
		
		



		auto debugRenderer = services::debugRendering();
		debugRenderer->viewProjectionMatrix(pv);
		debugRenderer->axis   (mat44f::scale(vec3f(3,3,3)));
		for(uint32 j = 0;j<boxCount;++j){
			debugRenderer->wireBox(mat44f::identity(),boxes[j].min,boxes[j].max,vec4f(0.5,0.0,0.5,1.0));
		}
		//debugRenderer->wireBox(mat44f::identity(),vec3f(0,1.0f,1.0f),vec3f(1,1.8f,1.6f),vec4f(0.5,0.0,0.5,1.0));
		debugRenderer->render (debugPipeline->pipeline());

		rendering::texture::Descriptor2D desc;
		auto ts = depthBuffer.getTexels(desc);
		auto depthTexture = renderer->create(desc,ts);

		char fps[128];
		sprintf(fps,"Dt = %f,%f,%d,%s",services::timing()->dt(),profRasterizeTiles.currentTime(),cov,vis?"true":"false");
		text.string_ = core::Bytes((void*)fps,strlen(fps));
		services::ui()->enterLayer(depthTexture);
		services::ui()->drawWidgets();
		services::ui()->render();

		renderer->release(depthTexture);
		services::rendering()->context()->swapBuffers();
		services::postStep();
	}
	services::shutdown();
	return 0;
}
