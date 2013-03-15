//NB: adapted from http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash2.h

//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
#pragma once

#include "../types.h"

namespace core {
	uint32 murmurHash32( const void * key, int len, uint32 seed );
}

//-----------------------------------------------------------------------------
