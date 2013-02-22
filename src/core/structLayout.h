#pragma once

#ifdef _MSC_VER
    #define STRUCT_PREALIGN(x) __declspec(align(x))
	#define STRUCT_POSTALIGN(x) 
	#define STRUCT_PREPACK
	#define STRUCT_POSTPACK
#elif __GNUC__
	#define STRUCT_PREALIGN(x)
    #define STRUCT_POSTALIGN(x) __attribute__ ((aligned (x)))
	#define STRUCT_PREPACK
	#define STRUCT_POSTPACK __attribute__ ((__packed__))
#else
	#error "Unsupported compiler!"
#endif


