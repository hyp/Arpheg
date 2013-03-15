#pragma once

#include <new>
#include "../core/math.h"
#include "../core/assert.h"
#include "types.h"
#include "events.h"
#include "components.h"

namespace ui {

class Widget {
public:
	inline bool isEnabled() const;
	inline bool isVisible() const;
	inline bool isHovered() const;
	inline bool isSelected() const;
	void show();
	void hide();
	void enable();
	void disable();
	void select();
	void deselect();
	void onClick();

	inline float x() const;
	inline float y() const;
	inline float width() const;
	inline float height() const;
	inline vec2f position() const;
	inline vec2f size() const;
	
	void* allocateComponent(size_t sz,size_t align);
	void addComponent(Component* component);
	void removeComponent(Component* component);
	inline Component** components();
	inline uint32 componentCount();

	void draw(events::Draw& ev);
	
	void onMouseMove(events::Mouse& ev);
	void onMouseButton(events::MouseButton& ev);
	void onMouseWheel(const events::MouseWheel& ev);
	void onKey(const events::Key& ev);
	void onJoystick(const events::Joystick& ev);
	void onTouch(events::Touch& ev);
	
	vec2f position_;
	vec2f size_;
	uint32 state_;
	uint32 color_;
	Widget* next_;
	uint32 componentCount_;
	enum { kSmallComponentArray = 3 };
	Component* components_[kSmallComponentArray];

	enum {
		StateVisible = 0x1,
		StateEnabled = 0x2,

		StateHover = 0x4,
		StateSelected = 0x8,
	};

	Widget(vec2f position = vec2f(0.f,0.f),vec2f size = vec2f(0.f,0.f));
};
inline bool Widget::isEnabled() const { return (state_ & StateEnabled)!=0; }
inline bool Widget::isVisible() const { return (state_ & StateVisible)!=0; }
inline bool Widget::isHovered() const { return (state_ & StateHover)!=0; }
inline bool Widget::isSelected() const { return (state_ & StateSelected)!=0; }
inline vec2f Widget::position() const { return position_; }
inline vec2f Widget::size() const { return size_; }
inline float Widget::x() const { return position_.x; }
inline float Widget::y() const { return position_.y; }
inline float Widget::width() const { return size_.x; }
inline float Widget::height() const { return size_.y; }
inline Component** Widget::components() { return componentCount_ <= kSmallComponentArray? components_: (Component**)components_[0]; }
inline uint32 Widget::componentCount()  { return componentCount_; }

}

namespace ui {
namespace internal_ {
	template<typename T>
	inline static T& ref(T& v) { return v; }
	template<typename T>
	inline static T& ref(T* v) { return *v; }
	template<typename T>
	inline static const T& ref(const T* v) { return *v; }
} }

// Sample usage:
// ARPHEG_UI_CONNECT(button,clicked(),this,doSomething())
// ARPHEG_UI_CONNECT(checkbox,selected(),this,turnSoundOn())
// ARPHEG_UI_CONNECT(checkbox,deselected(),this,turnSoundOff())
// ARPHEG_UI_CONNECT(volumeControl,modified(float volume),this,setVolume(volume))
// ARPHEG_UI_CONNECT(qualityList,selected(ui::String str),this,setQuality(str))
// NB: Requires C++11 decltype support
#define ARPHEG_UI_CONNECT(widget,event,object,f) {\
	class Handler : public ::ui::Component { public: typedef decltype(::ui::internal_::ref(object)) SelfRef; SelfRef self; inline Handler(SelfRef r):self(r){} void event { self.f; } }; \
	::ui::internal_::ref(widget).addComponent(new(::ui::internal_::ref(widget).allocateComponent(sizeof(Handler),alignof(Handler))) Handler(::ui::internal_::ref(object))); \
}
#define ARPHEG_UI_CONNECT_STATIC(widget,event,f) {\
	class Handler : public ::ui::Component { public: void event { f; } }; \
	::ui::internal_::ref(widget).addComponent(new(::ui::internal_::ref(widget).allocateComponent(sizeof(Handler),alignof(Handler))) Handler()); \
}
