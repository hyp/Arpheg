#include "../core/math.h"
#include "../core/assert.h"
#include "types.h"

namespace rendering {

	Camera::Camera(const mat44f& projectionMatrix,const mat44f& viewMatrix){
		projection = projectionMatrix;
		view = viewMatrix;
		projectionView = projection * view;
	}
	void Light::makeDirectional(vec3f direction,vec3f diffuse,vec3f ambient){
		parameterStorage_[0] = vec4f(direction.x,direction.y,direction.z,0.0f);
		parameterStorage_[1] = vec4f(diffuse.x,diffuse.y,diffuse.z,0.0f);
		parameterStorage_[2] = vec4f(ambient.x,ambient.y,ambient.z,0.0f);
		parameterStorage_[3] = vec4f(0.0f);
	}
	void Light::makePoint(vec3f position,vec3f diffuse,vec3f ambient,float radius,float constantAttenuation,float linearAttenuation,float quadraticAttenuation){
		assert(radius > 0.0f);
		parameterStorage_[0] = vec4f(position.x,position.y,position.z,radius);
		parameterStorage_[1] = vec4f(diffuse.x,diffuse.y,diffuse.z,0.0f);
		parameterStorage_[2] = vec4f(constantAttenuation,linearAttenuation,quadraticAttenuation,ambient.x);
		parameterStorage_[3] = vec4f(0.0f);
	}
	void Light::makeSpotLight  (vec3f position,vec3f direction,vec3f diffuse,vec3f ambient,float radius,float constantAttenuation,float linearAttenuation,float quadraticAttenuation,float innerCutoff,float outerCutoff){
		assert(radius > 0.0f);
		parameterStorage_[0] = vec4f(position.x,position.y,position.z,radius);
		parameterStorage_[1] = vec4f(diffuse.x,diffuse.y,diffuse.z,outerCutoff);
		parameterStorage_[2] = vec4f(constantAttenuation,linearAttenuation,quadraticAttenuation,ambient.x);
		parameterStorage_[3] = vec4f(direction.x,direction.y,direction.z,innerCutoff);
	}

namespace blending {
	State disabled() {
		return State();
	}
	State alpha(){
		return State(SrcAlpha,InvertedSrcAlpha);
	}
	State premultipliedAlpha(){
		return State(One,InvertedSrcAlpha);
	}
}

	//Provide descriptors for some common vertex formats
	core::TypeDescriptor positionAs2Float[1] = { { core::TypeDescriptor::TFloat,2 } };
	core::TypeDescriptor positionAs3Float[1] = { { core::TypeDescriptor::TFloat,3 } };
	core::TypeDescriptor positionAs4Float[1] = { { core::TypeDescriptor::TFloat,4 } };
	core::TypeDescriptor positionAs2Float_texcoordAs2Float[2] = { { core::TypeDescriptor::TFloat,2 } , { core::TypeDescriptor::TFloat,2 } };
	core::TypeDescriptor positionAs3Float_texcoordAs2Float[2] = { { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TFloat,2 } };
	core::TypeDescriptor positionAs2Float_texcoordAs2Float_colourAs4Bytes[3] = { 
		{ core::TypeDescriptor::TFloat,2 } , { core::TypeDescriptor::TFloat,2 } , { core::TypeDescriptor::TUint8,4 } };
	core::TypeDescriptor positionAs3Float_normalAs3Float[2] = { { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TFloat,3 } };
	core::TypeDescriptor positionAs4Float_normalAs4Float[2] = { { core::TypeDescriptor::TFloat,4 } , { core::TypeDescriptor::TFloat,4 } };
	core::TypeDescriptor positionAs3Float_normalAs3Float_texcoordAs2Float[3] = { { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TFloat,3 } , { core::TypeDescriptor::TFloat,2 } };

	const char* posSlots[]   = { "position" };
	const char* posUvSlots[] = { "position","texcoord" };
	const char* posNSlots[]  = { "position","normal" };
	const char* posNUVSlots[] = { "position","normal","texcoord" };
	const char* posUvColSlots[] = { "position","texcoord","colour" };

#define ImplCommonDescriptor(fields,slots) \
	VertexDescriptor VertexDescriptor::fields() { \
		VertexDescriptor result = { ::rendering:: fields,sizeof( ::rendering:: fields)/sizeof(core::TypeDescriptor),slots }; \
		return result; \
	}

	ImplCommonDescriptor(positionAs2Float,posSlots)
	ImplCommonDescriptor(positionAs3Float,posSlots)
	ImplCommonDescriptor(positionAs4Float,posSlots)
	ImplCommonDescriptor(positionAs2Float_texcoordAs2Float,posUvSlots)
	ImplCommonDescriptor(positionAs3Float_texcoordAs2Float,posUvSlots)
	ImplCommonDescriptor(positionAs2Float_texcoordAs2Float_colourAs4Bytes,posUvColSlots)
	ImplCommonDescriptor(positionAs3Float_normalAs3Float,posNSlots)
	ImplCommonDescriptor(positionAs3Float_normalAs3Float_texcoordAs2Float,posNUVSlots)
	ImplCommonDescriptor(positionAs4Float_normalAs4Float,posNSlots)

#undef ImplCommonDescriptor
}