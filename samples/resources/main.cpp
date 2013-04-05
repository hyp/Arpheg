#include <stdio.h>
#include <string.h>
#include "../rootPath.h"
#include "../../src/arpheg.h"
#include "../../src/services.h"
#include "../../src/data/data.h"

#include "../../src/rendering/text.h"
#include "../../src/rendering/debug.h"
#include "../../src/ui/ui.h"
#include "../../src/ui/components.h"
#include "../../src/ui/widget.h"
#include "../../src/rendering/softwareOcclusion/softwareOcclusion.h"
#include "../../src/rendering/animation.h"
#include "../../src/application/profiling.h"
#include "../../src/rendering/3d.h"
#include "../../src/scene/types.h"
#include "../../src/scene/rendering.h"


#include "../../src/rendering/opengl/gl.h"

int main(){

	services::init();
	services::logging()->priority(application::logging::Trace);
	services::application()->mainWindow()->create("Arpheg resources example");



	auto renderer = services::rendering();
	auto data     = services::data();
	
	rendering::Pipeline::Constant mvp("mvp");
	rendering::Pipeline::Constant textureConst("texture");

	//Data
	data->loadBundle(ARPHEG_ROOT_PATH "data/core/core.txt");
	auto repeatSampler = data->sampler("rendering.sampler.repeatLinear");
	data->loadBundle(ARPHEG_ROOT_PATH "data/sample.txt");

	ui::Image image(data->sprite("icon"));
	ui::Widget imageWidget(vec2i(0,0),vec2i(64,64));
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

	ui::TextureView tview;
	ui::FillLayout tlayout;
	ui::Widget depthView;depthView.addComponent(&tview);depthView.addComponent(&tlayout);
	services::ui()->root()->addChild(&depthView);



	vec4f vertices[8];
	math::utils::gatherBoxVertices(vertices,vec3f(-1.0f,-1.0f,-1.0f),vec3f(1.0f,1.0f,1.0f));
	uint16 ind[36];
	
	uint16* indices = ind;
	indices[0] = 2; indices[1] = 1; indices[2] = 0; indices[3] = 0;indices[4] = 3;indices[5] = 2;indices+=6;
	indices[0] = 4; indices[1] = 5; indices[2] = 6; indices[3] = 6;indices[4] = 7;indices[5] = 4;indices+=6;
	indices[0] = 4; indices[1] = 3; indices[2] = 0; indices[3] = 7;indices[4] = 3;indices[5] = 4;indices+=6;
	indices[0] = 1; indices[1] = 2; indices[2] = 5; indices[3] = 5;indices[4] = 2;indices[5] = 6;indices+=6;
	indices[0] = 0; indices[1] = 1; indices[2] = 5; indices[3] = 0;indices[4] = 5;indices[5] = 4;indices+=6;
	indices[0] = 6; indices[1] = 2; indices[2] = 3; indices[3] = 3;indices[4] = 7;indices[5] = 6;indices+=6;

	auto vbo = renderer->create(rendering::Buffer::Vertex,false,sizeof(vertices),vertices);
	auto ibo = renderer->create(rendering::Buffer::Index,false,sizeof(ind),ind);
	auto cubeMesh = renderer->create(vbo,ibo,rendering::VertexDescriptor::positionAs4Float());
	auto cubePipe = data->pipeline("raymarch");
	rendering::Pipeline::Constant cubePipeMvp("mvp");
	rendering::Pipeline::Constant cubePipeMv("mv");
	rendering::Pipeline::Constant cubeTexture("flameTexture");

	auto mesh    = data->mesh("head");
	auto foo = data->mesh("foo");
	auto staticMeshPipeline = data->pipeline("staticMeshPipeline");
	auto animation = data->animation("head.animation.0");
	auto program = data->pipeline("default");
	auto font    = data->font("fonts.lily64px");
	auto fontOutlined    = data->font("font");

	ui::Text text("Hello",font,0xFFFFFFFF,0xFF0000FF);
	ui::Widget textWidget(vec2i(32,402),vec2i(100,400));
	textWidget.addComponent(&text);
	services::ui()->root()->addChild(&textWidget);

	rendering::softwareOcclusion::DepthBuffer depthBuffer(vec2i(1920/8,1080/6));


	application::profiling::Timer profDepthClear("Clear depth");
	application::profiling::Timer profBinBoxes("Bin Boxes");
	application::profiling::Timer profRasterizeTiles("Rasterize Tiles");
	application::profiling::Timer profQuery("Query occlusion");

	float cam = 3.f;
	float anim = 0.f;

	auto ent = services::sceneRendering()->create(foo,foo->submesh(0)->material(),vec3f(20,0,0),Quaternion::identity(),vec3f(1.0f,2.0f,1.0f));
	
	for(int x = 0;x<5;++x){
	for(int y = 0;y<5;++y){
	for(int z = 0;z<5;++z){
		auto human = services::sceneRendering()->create(mesh,mesh->submesh(0)->material(),vec3f(float(x)*2.0f,float(y)*2.0f,float(z)*2.0f),Quaternion::rotateY(math::pi/4.0),vec3f(1.0,1.0,1.0));
		services::sceneRendering()->addAnimation(human,animation);
	} } }
	application::profiling::Timer profAnim("Animation interpolation");


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

		rendering::Camera camera( mat44f::perspective(math::pi/2,float(viewport.size.x)/float(viewport.size.y),0.1,1000.0),
								  mat44f::lookAt     (vec3f(cam,cam,cam),vec3f(0,0,0),vec3f(0,1,0)) );

		renderer->bind(mvp,camera.calculateMvp(mat44f::identity()));


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
		
		services::debugRendering()->skeleton(mat44f::identity(),mesh,nodes);
		rendering::Pipeline::Constant c("boneMatrices");
		for(size_t i = 0;i<mesh->submeshCount();++i){
			auto submesh = mesh->submesh(i);

			//auto bones = animator->transformationBuffer().allocate(mesh->skeletonNodeCount());
			//rendering::animation::Animator::bindSkeleton(submesh,mesh->skeletonNodeCount(),nodes,bones);
			//renderer->bind(c,(void*)bones);
			
			if(auto mat= submesh->material()){
				uint32 slots[::data::Material::kMaxTextures];
				for(uint32 i = 0;i<mat->textureCount();++i){
					renderer->bind(mat->textures()[i],i);
					slots[i] = i;
				}
				renderer->bind(textureConst,slots);
			}

			renderer->bind(submesh->mesh(),submesh->primitiveKind(),submesh->indexSize());
			//renderer->drawIndexed(submesh->primitiveOffset(),submesh->primitiveCount());
		}

		rendering::DirectMeshRenderer dmr;
		dmr.bind(camera);
		dmr.bind(staticMeshPipeline,program);
		scene::events::Draw ev; ev.meshRenderer = &dmr;
		services::sceneRendering()->setActiveCameras(&camera,1);
		services::sceneRendering()->spawnFrustumCullingTasks();
		profAnim.start();
		services::sceneRendering()->spawnAnimationTasks();
		profAnim.end();
		services::sceneRendering()->render(ev);
		

		
		
		/*glEnable (GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		renderer->bind(rendering::blending::alpha());
		renderer->bind(cubePipe->pipeline());
		renderer->bind(cubePipeMvp,pv);
		renderer->bind(cubePipeMv,view);
		static float currentTime = 0.f;
		currentTime+=services::timing()->dt();
		renderer->bind(rendering::Pipeline::Constant("currentTime"),&currentTime);
		int xxx = 0;
		auto t = data->texture2D("flameTexture");
		//renderer->bind(repeatSampler,0);
		renderer->bind(t,0);
		renderer->bind(cubeTexture,&xxx);
		renderer->bind(cubeMesh,rendering::topology::Triangle,sizeof(uint16));
		renderer->drawIndexed(0,36);*/


		auto debugRenderer = services::debugRendering();
		debugRenderer->node   (mat44f::scale(vec3f(3,3,3)));
		for(uint32 j = 0;j<boxCount;++j){
			debugRenderer->box(mat44f::identity(),boxes[j].min,boxes[j].max,vec4f(0.5,0.0,0.5,1.0));
		}
		debugRenderer->render (pv);

		rendering::texture::Descriptor2D desc;
		auto ts = depthBuffer.getTexels(desc);
		tview.texture_  = renderer->create(desc,ts);
		tview.pipeline_ = services::data()->pipeline(services::data()->bundle("core",true),"rendering.visualize.depthBuffer.pipeline",true)->pipeline();

		char fps[128];
		sprintf(fps,"Dt = %f,%f,%d,%s,%d",services::timing()->dt(),profRasterizeTiles.currentTime(),cov,vis?"true":"false",services::sceneRendering()->renderedEntityCount());
		text.string_ = core::Bytes((void*)fps,strlen(fps));
		
		services::ui()->drawWidgets();
		services::ui()->render();

		renderer->release(tview.texture_);
		services::rendering()->context()->swapBuffers();
		services::postStep();
	}
	services::shutdown();
	return 0;
}
