#pragma once

#include "types.h"

namespace core {

struct TypeDescriptor {
	enum Type {
		TVoid,TInt8,TUint8,TInt16,TUint16,TInt32,TUint32,TInt64,TUint64,
		THalf,TFloat,TDouble,
		TMat44Float,TMat33Float,
	};
	uint8 id;
	uint8 count;

	uint32 size();
	bool operator == (TypeDescriptor other) const;
};

}
