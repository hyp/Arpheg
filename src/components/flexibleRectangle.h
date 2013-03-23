/*
# This component is a rectangle which can be specified in terms of fixed position/size and/or relative to parent position/size.
*/
#pragma once

#include "../core/math.h"

namespace components {
	struct FlexibleRectangle {
		vec4f relativeRect;
		vec4i fixedRect;

		inline FlexibleRectangle () {}
		inline FlexibleRectangle(vec2i position,vec2i size) : 
			relativeRect(0.0f,0.0f,0.0f,0.0f),fixedRect(position.x,position.y,size.x,size.y) {}
		inline FlexibleRectangle(vec2f position,vec2f size) :
			relativeRect(position.x,position.y,size.x,size.y),fixedRect(0,0,0,0) {}
		inline FlexibleRectangle(vec2f position,vec2i size) :
			relativeRect(position.x,position.y,0.0f,0.0f),fixedRect(0,0,size.x,size.y) {}
		inline FlexibleRectangle(vec2f position,vec2i offset,vec2i size) :
			relativeRect(position.x,position.y,0.0f,0.0f),fixedRect(offset.x,offset.y,size.x,size.y) {}

		inline vec4i calculate(vec2i parentSize) {
			vec4f relativeMultiplier(float(parentSize.x),float(parentSize.y),float(parentSize.x),float(parentSize.y));
			return vec4i(relativeRect * relativeMultiplier) + fixedRect;
		}
	};
}
