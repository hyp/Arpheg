#include "typeDescriptor.h"

uint32 CoreTypeDescriptor::size() {
	uint32 element;
	if(id >= TInt8 && id <= TUint64) {
		element = 1<<((id-TInt8)/2);
	}
	else if(id >= THalf && id <= TDouble) {
		element = 2<<(id-THalf);
	}
	else if(id == TVoid) element = 0;
	return element*count;
}
