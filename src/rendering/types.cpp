#include "../core/math.h"
#include "types.h"

namespace rendering {

	//Provide descriptors for some common vertex formats
	CoreTypeDescriptor positionAs2Float[1] = { { CoreTypeDescriptor::TFloat,2 } };
	CoreTypeDescriptor positionAs3Float[1] = { { CoreTypeDescriptor::TFloat,3 } };
	CoreTypeDescriptor positionAs4Float[1] = { { CoreTypeDescriptor::TFloat,4 } };
	CoreTypeDescriptor positionAs2Float_texcoordAs2Float[2] = { { CoreTypeDescriptor::TFloat,2 } , { CoreTypeDescriptor::TFloat,2 } };
	CoreTypeDescriptor positionAs3Float_texcoordAs2Float[2] = { { CoreTypeDescriptor::TFloat,3 } , { CoreTypeDescriptor::TFloat,2 } };
	CoreTypeDescriptor positionAs2Float_texcoordAs2Float_colourAs4Bytes[3] = { 
		{ CoreTypeDescriptor::TFloat,2 } , { CoreTypeDescriptor::TFloat,2 } , { CoreTypeDescriptor::TUint8,4 } };
	CoreTypeDescriptor positionAs3Float_normalAs3Float[2] = { { CoreTypeDescriptor::TFloat,3 } , { CoreTypeDescriptor::TFloat,3 } };
	CoreTypeDescriptor positionAs4Float_normalAs4Float[2] = { { CoreTypeDescriptor::TFloat,4 } , { CoreTypeDescriptor::TFloat,4 } };

	const char* posSlots[]   = { "position" };
	const char* posUvSlots[] = { "position","texcoord" };
	const char* posNSlots[]  = { "position","normal" };
	const char* posUvColSlots[] = { "position","texcoord","colour" };

#define ImplCommonDescriptor(fields,slots) \
	VertexDescriptor VertexDescriptor::fields() { \
		VertexDescriptor result = { ::rendering:: fields,sizeof( ::rendering:: fields)/sizeof(CoreTypeDescriptor),slots }; \
		return result; \
	}

	ImplCommonDescriptor(positionAs2Float,posSlots)
	ImplCommonDescriptor(positionAs3Float,posSlots)
	ImplCommonDescriptor(positionAs4Float,posSlots)
	ImplCommonDescriptor(positionAs2Float_texcoordAs2Float,posUvSlots)
	ImplCommonDescriptor(positionAs3Float_texcoordAs2Float,posUvSlots)
	ImplCommonDescriptor(positionAs2Float_texcoordAs2Float_colourAs4Bytes,posUvColSlots)
	ImplCommonDescriptor(positionAs3Float_normalAs3Float,posNSlots)
	ImplCommonDescriptor(positionAs4Float_normalAs4Float,posNSlots)
}