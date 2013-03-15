//NB: adapted from http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash2.cpp

//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.


#include "murmur.h"


//-----------------------------------------------------------------------------
// Platform-specific functions and macros


// Microsoft Visual Studio


#if defined(_MSC_VER)


#define BIG_CONSTANT(x) (x)


// Other compilers


#else   // defined(_MSC_VER)


#define BIG_CONSTANT(x) (x##LLU)


#endif // !defined(_MSC_VER)


//-----------------------------------------------------------------------------
// MurmurHashNeutral2, by Austin Appleby


// Same as MurmurHash2, but endian- and alignment-neutral.
// Half the speed though, alas.

uint32 core::murmurHash32 ( const void * key, int len, uint32 seed ) {
	const uint32 m = 0x5bd1e995;
	const int r = 24;


	uint32 h = seed ^ len;


	const unsigned char * data = (const unsigned char *)key;


	while(len >= 4)
	{
		uint32 k;


		k  = data[0];
		k |= data[1] << 8;
		k |= data[2] << 16;
		k |= data[3] << 24;


		k *= m; 
		k ^= k >> r; 
		k *= m;


		h *= m;
		h ^= k;


		data += 4;
		len -= 4;
	}
  
	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
			h *= m;
	};


	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}
