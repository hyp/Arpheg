#pragma once

#include "../data/types.h"
#include "types.h"
#include "events.h"

namespace ui {

class Component {
public:
	Component();
	virtual ~Component();
	virtual void draw(Widget* widget,events::Draw& ev);
	virtual void onMouseMove(Widget* widget,events::Mouse& ev);
	virtual void onMouseButton(Widget* widget,events::MouseButton& ev);
	virtual void onMouseWheel(Widget* widget,const events::MouseWheel& ev);
	virtual void onKey(Widget* widget,const events::Key& ev);
	virtual void onJoystick(Widget* widget,const events::Joystick& ev);
	virtual void onTouch(Widget* widget,events::Touch& ev);
	virtual void onResize(Widget* widget,const vec2i& size);
	virtual void onLayout(Widget* widget,events::Layout& layout);
	
	//Connectabled slots.
	virtual void clicked();	
	virtual void selected();
	virtual void deselected();
	virtual void modified(float x);
	virtual void shown();
	virtual void hidden();
	virtual void enabled();
	virtual void disabled();
	
	virtual vec2i calculateSize();
	virtual vec2i calculateBorder();
protected:
	uint32 flags_;
};

//Layouts.
class Layout: public Component {
public:
	Layout();
	void  contentsMargins(int32 left,int32 top,int32 right,int32 bottom);
	inline vec4i contentsMargins() const;
	void spacing(int32 space);
	void spacing(vec2i space);
	inline vec2i spacing() const;

private:
	vec4i contentsMargins_;//TODO: replace by int[4]
	vec2i spacing_;
};
inline vec4i Layout::contentsMargins() const { return contentsMargins_; }
inline vec2i Layout::spacing() const { return spacing_; }

class FillLayout: public Layout {
public:
	void onLayout(Widget* widget,events::Layout& layout);
};


class Renderable: public Component {
public:
	enum {
		FlagIsOpaque = 0x100,
	};
	Renderable();
	inline bool isOpaque() const;
protected:
	void checkOpaque(uint32 colour);
	void checkOpaque(const uint32* colours,uint32 n);
};

class Group: public Renderable {
public:
	Group();

	void addChild(Widget* child);
	void draw(Widget* widget,events::Draw& ev);
	
	void onMouseMove(Widget* widget,events::Mouse& ev);
	void onMouseButton(Widget* widget,events::MouseButton& ev);
	void onMouseWheel(Widget* widget,const events::MouseWheel& ev);
	void onKey(Widget* widget,const events::Key& ev);
	void onJoystick(Widget* widget,const events::Joystick& ev);
	void onTouch(Widget* widget,events::Touch& ev);
	void onResize(Widget* widget,const vec2i& size);
	
	vec2i calculateSize();

	uint32 childrenCount_;
	Widget* children_;
};
class Image: public Renderable {
public:
	typedef const data::Sprite* DataType;

	Image(DataType image,uint32 colour = 0xFFffFFff);
	void draw(Widget* widget,events::Draw& ev);
	vec2i calculateSize();

	const data::Sprite* image_;
	uint32 color_;
};
class TextureView: public Renderable {
public:
	TextureView(rendering::Texture2D texture = rendering::Texture2D::null(),rendering::Pipeline pipeline = rendering::Pipeline::nullPipeline());

	void draw(Widget* widget,events::Draw& ev);

	rendering::Pipeline  pipeline_;
	rendering::Texture2D texture_;
	uint32 color_;
};
class Rectangle: public Renderable {
public:
	Rectangle(uint32 colour = 0xFFffFFff);
	void makeFlat(uint32 colour);
	void makeTopDownGradient(uint32 topColour,uint32 bottomColour);
	void makeLeftRightGradient(uint32 leftColour,uint32 rightColour);
	void draw(Widget* widget,events::Draw& ev);
	
	uint32 colors_[4];
};
class RectangularBorder: public Renderable {
	RectangularBorder(uint32 colour = 0xFFffFFff,int32 thickness = 1);
	void draw(Widget* widget,events::Draw& ev);

	uint32 colors_[4];
	int32  thickness_[4];
};
class Text: public Renderable {
public:
	typedef data::Font* FontType;

	Text(const char* string,FontType font,uint32 colour = 0xFFffFFff,uint32 outlineColour = 0xFFffFFff);
	Text(core::Bytes string,FontType font,uint32 colour = 0xFFffFFff,uint32 outlineColour = 0xFFffFFff);
	void draw(Widget* widget,events::Draw& ev);
	vec2i calculateSize();

	core::Bytes string_;
	data::Font* font_;
	uint32 color_;
	uint32 outlineColor_;
};

// Button style
class Clickable: public Component {
public:
	void onMouseButton(Widget* widget,events::MouseButton& ev);
	void onTouch(Widget* widget,events::Touch& ev);
};
// Checkbox style
class Selectable: public Clickable {
public:
	void onMouseButton(Widget* widget,events::MouseButton& ev);
	void onTouch(Widget* widget,events::Touch& ev);
};
	

}