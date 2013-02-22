#pragma once

#include "types.h"
#include "structLayout.h"
#include "math.h"

namespace simd {

#ifdef ARPHEG_ARCH_X86

	typedef __m128  float4;
	typedef __m128  bool4;
	
	inline float4 add(float4 x,float4 y) { return _mm_add_ps(x,y); }
	inline float4 sub(float4 x,float4 y) { return _mm_sub_ps(x,y); }
	inline float4 mul(float4 x,float4 y) { return _mm_mul_ps(x,y); }
	inline float4 div(float4 x,float4 y) { return _mm_div_ps(x,y); }
	inline float4 sqrt(float4 x)         { return _mm_rsqrt_ps(x); }
	inline float4 reciprocal(float4 x)   { return _mm_rcp_ps(x);   }
	inline float4 reciprocalSqrt(float4 x) { return _mm_rsqrt_ps(x); }
	inline float4 max(float4 x,float4 y) { return _mm_max_ps(x,y); }
	inline float4 min(float4 x,float4 y) { return _mm_min_ps(x,y); }

	inline bool4 compareLessThan(float4 x,float4 y)      { return _mm_cmplt_ps(x,y); }
	inline bool4 compareLessEquals(float4 x,float4 y)    { return _mm_cmple_ps(x,y); }
	inline bool4 compareGreaterThan(float4 x,float4 y)   { return _mm_cmpgt_ps(x,y); }
	inline bool4 compareGreaterEquals(float4 x,float4 y) { return _mm_cmpge_ps(x,y); }

#endif

}
