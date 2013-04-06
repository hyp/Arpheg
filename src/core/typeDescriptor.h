#pragma once

#include "types.h"

namespace core {

struct TypeDescriptor {
	enum Type {
		TVoid,TInt8,TUint8,TInt16,TUint16,TInt32,TUint32,TInt64,TUint64,
		THalf,TFloat,TDouble,
		TMat44Float,TMat33Float,
		TInt3x10_2,TUint3x10_2, //Corresponds to GL_ARB_vertex_type_2_10_10_10_rev | GL_OES_vertex_type_10_10_10_2

		kNormalized = 0x80,

		TNormalizedUint8  = kNormalized|TUint8,
		TNormalizedInt8   = kNormalized|TInt8,
		TNormalizedUint16 = kNormalized|TUint16,
		TNormalizedInt16  = kNormalized|TInt16,
		TNormalizedInt3x10_2 = kNormalized|TInt3x10_2,
		TNormalizedUint3x10_2 = kNormalized|TUint3x10_2
	};
	uint8 id;
	uint8 count;

	uint32 size();
	bool operator == (TypeDescriptor other) const;
};

}
