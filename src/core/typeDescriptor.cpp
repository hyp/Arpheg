#include "assert.h"
#include "typeDescriptor.h"

namespace core {

uint32 TypeDescriptor::size() {
	uint32 element;
	if(id >= TInt8 && id <= TUint64) {
		element = 1<<((id-TInt8)/2);
	}
	else if(id >= THalf && id <= TDouble) {
		element = 2<<(id-THalf);
	}
	else if(id == TVoid) element = 0;
	else if(id == TMat44Float) element = sizeof(float)*16;
	else if(id == TMat33Float) element = sizeof(float)*9;
	else assert(false && "Invalid type id!");
	return element*count;
}
bool TypeDescriptor::operator == (TypeDescriptor other) const {
	return other.id == id && other.count == count;
}

}
