#include "../core/allocatorNew.h"
#include "../core/assert.h"
#include "../core/math.h"
#include "../services.h"
#include "../rendering/ui.h"
#include "../rendering/text.h"
#include "../rendering/2d.h"
#include "../input/input.h"
#include "../input/events.h"
#include "ui.h"
#include "widget.h"
#include "events.h"
#include "components.h"

namespace ui {


void Service::updateWidgets() {

	//Handles input for the ui hierarchy
	class InputHandler:public input::events::IHandler {
	public:
		Group* root;
		Widget* focused;

		void onMouseMove(const input::events::Mouse& ev){ 
			events::Mouse evc(ev);
			root->onMouseMove(nullptr,evc);
		}
		void onMouseButton(const input::events::MouseButton& ev){
			events::MouseButton evc(ev);
			root->onMouseButton(nullptr,evc);
		}
		void onMouseWheel(const input::events::MouseWheel& ev){
			if(focused) focused->onMouseWheel(ev);
		}
		void onKey(const input::events::Key& ev) {
			if(focused) focused->onKey(ev);
		}
		void onJoystick(const input::events::Joystick& ev) {
			if(focused) focused->onJoystick(ev);
		}
		void onTouch(const input::events::Touch& ev) {
			events::Touch evc(ev);
			root->onTouch(nullptr,evc);
		}
	};
	InputHandler handler;
	handler.root = root_;
	handler.focused = focused_;
	services::input()->handleEvents(&handler);

	//TODO: Handle application events - resize/activate/deactivate?
}
void Service::drawWidgets() {
	core::BufferAllocator glyphBuffer(sizeof(rendering::text::Glyph)*128,&services::tasking()->threadContext().frameAllocator(),core::BufferAllocator::GrowOnOverflow);	
	
	events::Draw drawEvent;
	drawEvent.layerId = 0;
	drawEvent.renderer = renderer_;
	drawEvent.glyphExtractionBuffer = &glyphBuffer;
	drawEvent.position = vec2i(0,0);
	auto size = services::rendering()->context()->frameBufferSize();
	drawEvent.size = vec2i(size.x,size.y);
	Widget widget;
	widget.addComponent(root_);
	widget.draw(drawEvent);
}

void Service::setFocus(Widget* widget) {
	focused_ = widget;
}
void Service::loadData(data::ID bundle) {
	auto data = services::data();
	//TODOdata->switchBundle(bundle);
	uiPipeline = data->pipeline("ui.pipeline");
	uiAtlas = data->texture2D("ui.atlas");
	textPipeline = data->pipeline("text.pipeline");
	outlineTextPipeline = data->pipeline("text.outlined.pipeline");
}

void Service::render() {
	auto size = services::rendering()->context()->frameBufferSize();
	mat44f matrix = mat44f::ortho(vec2f(0,0),vec2f(float(size.x),float(size.y)));
	renderer_->render(matrix);
}
core::Allocator* Service::componentAllocator() const {
	return core::memory::globalAllocator();
}
void Service::enterLayer(rendering::Texture2D t) {
	assert(uiPipeline && "UI data not loaded!");
	using namespace rendering::ui;

	Batch batch;
	batch.vertexLayout = rendering::draw2D::positionInt16::vertexLayout(rendering::draw2D::mode::Textured|rendering::draw2D::mode::Coloured);
	batch.name = "uiMainBatch";
	Batch::Material material;
	material.pipeline = uiPipeline->pipeline();
	material.matrix   = rendering::Pipeline::Constant("matrix");
	material.residentTextures = true;
	material.textureCount = 1;
	material.textures[0].texture = t;
	material.textures[0].constant = rendering::Pipeline::Constant("texture");
	renderer_->registerBatch(batch,material);
	//assert(kTexturedColouredTrianglesBatch == );
	
	batch.vertexLayout = rendering::draw2D::positionInt16::vertexLayout(rendering::draw2D::mode::Coloured);
	batch.name = "Coloured triangles";
	material.pipeline = uiPipeline->pipeline();
	material.matrix   = rendering::Pipeline::Constant("matrix");
	material.residentTextures = false;
	material.textureCount = 0;	
	renderer_->registerBatch(batch,material);
	//assert(kColouredTrianglesBatch == );
}
void Service::servicePreStep(){
	renderer_->servicePreStep();
	
	renderer_->registerFontPipelines(textPipeline->pipeline(),outlineTextPipeline->pipeline());
	//enterLayer();
}
void Service::servicePostStep(){
	renderer_->servicePostStep();
}
Service::Service(core::Allocator* allocator){
	renderer_ = ALLOCATOR_NEW(allocator,rendering::ui::Service);
	uiPipeline = nullptr;
	root_ = ALLOCATOR_NEW(allocator,Group);
	focused_ = nullptr;
}
Service::~Service(){
	renderer_->~Service();
	root_->~Group();
}

}