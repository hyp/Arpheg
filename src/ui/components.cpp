#include <string.h>
#include "../core/bufferArray.h"
#include "../rendering/ui.h"
#include "../rendering/text.h"
#include "../rendering/2d.h"
#include "ui.h"
#include "components.h"
#include "widget.h"

namespace ui {

Component::Component() :flags_(0) {}
Component::~Component() {}
void Component::draw(Widget* widget,events::Draw& ev) { }
void Component::onMouseMove(Widget* widget,events::Mouse& ev) { }
void Component::onMouseButton(Widget* widget,events::MouseButton& ev) { }
void Component::onMouseWheel(Widget* widget,const events::MouseWheel& ev) { }
void Component::onKey(Widget* widget,const events::Key& ev) { }
void Component::onJoystick(Widget* widget,const events::Joystick& ev) { }
void Component::onTouch(Widget* widget,events::Touch& ev) {}
void Component::onResize(Widget* widget,const vec2i& size) {}

void Component::clicked(){ }
void Component::selected() { }
void Component::deselected() { }
void Component::modified(float x) { }
void Component::shown() { }
void Component::hidden() { }
void Component::enabled() { }
void Component::disabled() { }

vec2i Component::calculateSize(){
	return vec2i(0,0);
}
vec2i Component::calculateBorder(){
	return vec2i(0,0);
}
void Component::onLayout(Widget* widget,events::Layout& layout) {
}

Group::Group() : childrenCount_(0),children_(nullptr) {}
void Group::draw(Widget* widget,events::Draw& ev){
	for(auto i = children_;i!=nullptr;i = i->next_) i->draw(ev);
}
//Dispatch input events to children
#define UI_GROUP_DISPATCH_INPUT(event,T)\
void Group::event(Widget* widget,T& ev){ \
	for(auto i = children_;i!=nullptr;i = i->next_) i->event(ev); \
}
#define UI_GROUP_DISPATCH_INPUT_CONST(event,T)\
void Group::event(Widget* widget,const T& ev){ \
	for(auto i = children_;i!=nullptr;i = i->next_) i->event(ev); \
}

UI_GROUP_DISPATCH_INPUT(onMouseMove,events::Mouse)
UI_GROUP_DISPATCH_INPUT(onMouseButton,events::MouseButton)
UI_GROUP_DISPATCH_INPUT_CONST(onMouseWheel,events::MouseWheel)
UI_GROUP_DISPATCH_INPUT_CONST(onKey,events::Key)
UI_GROUP_DISPATCH_INPUT_CONST(onJoystick,events::Joystick)
UI_GROUP_DISPATCH_INPUT(onTouch,events::Touch)

#undef UI_GROUP_DISPATCH_INPUT
#undef UI_GROUP_DISPATCH_INPUT_CONST

void Group::onResize(Widget* widget,const vec2i& size) { 
	events::Layout layout;
	layout.parent = widget;
	layout.runningPosition = vec2i(0,0);
	for(auto i = children_;i!=nullptr;i = i->next_) i->onLayout(layout);
}
vec2i Group::calculateSize() {
	vec2i sz = vec2i(0,0);
	return sz;
}
void Group::addChild(Widget* child) {
	++childrenCount_;
	auto i = &children_;
	for(;(*i)!=nullptr;i = &((*i)->next_));
	*i = child;
}

void FillLayout::onLayout(Widget* widget,events::Layout& layout) {
	widget->move(vec2i(0,0),layout.parent->size());
}

Renderable::Renderable() {
}
inline bool Renderable::isOpaque() const {
	return (flags_&FlagIsOpaque)!=0;
}
static inline bool isColoursAlpha255(uint32 colour){
#ifndef ARPHEG_ARCH_BIG_ENDIAN
	return (colour&0xFF000000) == 0xFF000000;
#else
	return (colour&0x000000FF) == 0x000000FF;
#endif
}
void Renderable::checkOpaque(uint32 colour) {
	flags_&=(~FlagIsOpaque);
	if(isColoursAlpha255(colour)) flags_|=FlagIsOpaque;
}
void Renderable::checkOpaque(const uint32* colours,uint32 n){
	flags_&=(~FlagIsOpaque);
	for(uint32 i = 0;i<n;++i){
		if(!isColoursAlpha255(colours[i])) return;
	}
	flags_|=FlagIsOpaque;
}
	
Image::Image(DataType image,uint32 colour) : image_(image),color_(colour) {
}
void Image::draw(Widget* widget,events::Draw& ev) {
	auto geometry = ev.renderer->allocate(ev.layerId,uint32(Service::kTexturedColouredTrianglesBatch),ev.renderer->uiTexture(image_->texture_),4,6);
	rendering::draw2D::positionInt16::textured::coloured::quad(geometry,ev.position,ev.position + ev.size,image_->frames()[0].textureCoords,color_);
}
vec2i Image::calculateSize() {
	return image_->size();
}

static data::normalizedUint16::Type defaultTextureCoords[] = { 0,0, data::normalizedUint16::one , data::normalizedUint16::one };

TextureView::TextureView(rendering::Texture2D texture,rendering::Pipeline pipeline) : pipeline_(pipeline),texture_(texture) {}
void TextureView::draw(Widget* widget,events::Draw& ev) {
	if(texture_ == rendering::Texture2D::null()) return;

	uint32 batchId = uint32(Service::kTexturedColouredTrianglesBatch);
	if(!(pipeline_ == rendering::Pipeline::nullPipeline())){
		//Create a new batch for this texture.
		rendering::ui::Batch batch;
		batch.depth = 0;
		batch.name  = "TextureView";
		auto pipelineId = ev.renderer->uiPipeline(pipeline_);
		batchId = ev.renderer->registerBatch(batch,pipelineId);
	}
	auto geometry = ev.renderer->allocate(ev.layerId,batchId,ev.renderer->uiTexture(texture_),4,6);
	rendering::draw2D::positionInt16::textured::coloured::quad(geometry,ev.position,ev.position + ev.size,defaultTextureCoords,0xFFffFFff);	
}

Rectangle::Rectangle(uint32 colour){
	colors_[0] = colors_[1] = colors_[2] = colors_[3] = colour;
	checkOpaque(colour);
}
void Rectangle::makeFlat(uint32 colour) {
	colors_[0] = colors_[1] = colors_[2] = colors_[3] = colour;
	checkOpaque(colour);
}
void Rectangle::makeTopDownGradient(uint32 topColour,uint32 bottomColour){
	colors_[0] = topColour; colors_[1] = topColour;
	colors_[2] = bottomColour; colors_[3] = bottomColour;
	checkOpaque(colors_,4);
}
void Rectangle::makeLeftRightGradient(uint32 leftColour,uint32 rightColour){
	colors_[0] = leftColour; colors_[1] = rightColour;
	colors_[2] = leftColour; colors_[3] = rightColour; 
	checkOpaque(colors_,4);
}
void Rectangle::draw(Widget* widget,events::Draw& ev) {
	auto geometry = ev.renderer->allocate(ev.layerId,uint32(Service::kTexturedColouredTrianglesBatch),4,6);

	rendering::draw2D::positionInt16::textured::coloured::quad(geometry,ev.position,ev.position + ev.size,defaultTextureCoords,colors_);
}

RectangularBorder::RectangularBorder(uint32 colour,int32 thickness){
	for(uint32 i = 0;i<4;++i){
		colors_[i] = colour;
		thickness_[i] = thickness;
	}
	checkOpaque(colour);
}
void RectangularBorder::draw(Widget* widget,events::Draw& ev){
	//TODO
}

Text::Text(const char* string,FontType font,uint32 colour,uint32 outlineColour) : string_((void*)string,strlen(string)),font_(font) {
	color_ = colour;
	outlineColor_ = outlineColour;
}
Text::Text(core::Bytes string,FontType font,uint32 colour,uint32 outlineColour) : string_(string),font_(font) {
	color_ = colour;
	outlineColor_ = outlineColour;
}
void Text::draw(Widget* widget,events::Draw& ev){
	auto string = string_;
	core::BufferAllocator& glyphBuffer=*ev.glyphExtractionBuffer;
	auto stringPos = ev.position;
	while(!string.empty()){
		string = rendering::text::extractGlyphs(string,font_,glyphBuffer);
		uint32 count = core::bufferArray::length<rendering::text::Glyph>(glyphBuffer);
		auto geometry = ev.renderer->allocate(ev.layerId,font_,count*4,count*6);
		rendering::text::drawGlyphs(geometry,stringPos,font_,color_,outlineColor_,core::bufferArray::begin<rendering::text::Glyph>(glyphBuffer),count);
		stringPos.y+=int(font_->lineHeight());
		glyphBuffer.reset();
	}
}
vec2i Text::calculateSize(){
	return vec2i(0,0);//TODO
}	

void Clickable::onMouseButton(Widget* widget,events::MouseButton& ev){
	if(ev.button == input::mouse::Left && ev.action == input::mouse::Pressed)
		widget->onClick();
}
void Clickable::onTouch(Widget* widget,events::Touch& ev){
	widget->onClick();
}
void Selectable::onMouseButton(Widget* widget,events::MouseButton& ev){
	if(ev.button == input::mouse::Left && ev.action == input::mouse::Pressed){
		if(widget->isSelected()) widget->deselect(); else widget->select();
	}
}
void Selectable::onTouch(Widget* widget,events::Touch& ev){
	if(widget->isSelected()) widget->deselect(); else widget->select();
}


}