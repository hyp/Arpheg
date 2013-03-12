#pragma once

#include "../../core/math.h"

union VecS32 {
	int32 lane[4];
	__m128i simd;

	VecS32() {}
	explicit VecS32(int32 x) : simd(_mm_set1_epi32(x)) {}
	explicit VecS32(const __m128i &in) : simd(in) {}
	VecS32(const VecS32 &x) : simd(x.simd) {}
	VecS32(int32 a, int32 b, int32 c, int32 d) : simd(_mm_setr_epi32(a, b, c, d)) {}

	static VecS32 zero()							{ return VecS32(_mm_setzero_si128()); }
	static VecS32 load(const int32 *ptr)				{ return VecS32(_mm_load_si128((const __m128i *)ptr)); }
	static VecS32 loadu(const int32 *ptr)				{ return VecS32(_mm_loadu_si128((const __m128i *)ptr)); }
	void store(int32 *ptr)							{ _mm_store_si128((__m128i *)ptr, simd); }
	void storeu(int32 *ptr)							{ _mm_storeu_si128((__m128i *)ptr, simd); }

	VecS32 &operator =(const VecS32 &b)				{ simd = b.simd; return *this; }
	VecS32 &operator +=(const VecS32 &b)			{ simd = _mm_add_epi32(simd, b.simd); return *this; }
	VecS32 &operator -=(const VecS32 &b)			{ simd = _mm_sub_epi32(simd, b.simd); return *this; }
	VecS32 &operator |=(const VecS32 &b)			{ simd = _mm_or_si128(simd, b.simd); return *this; }
	VecS32 &operator &=(const VecS32 &b)			{ simd = _mm_and_si128(simd, b.simd); return *this; }
};

inline VecS32 operator +(const VecS32 &a, const VecS32 &b)		{ return VecS32(_mm_add_epi32(a.simd, b.simd)); }
inline VecS32 operator -(const VecS32 &a, const VecS32 &b)		{ return VecS32(_mm_sub_epi32(a.simd, b.simd)); }
inline VecS32 operator *(const VecS32 &a, const VecS32 &b)		{ 
//GODDAMNIT SSE
//http://stackoverflow.com/questions/10500766/sse-multiplication-of-4-32-bit-integers
#ifdef __SSE4_1__  // modern CPU - use SSE 4.1
    return VecS32(_mm_mullo_epi32(a.simd, b.simd));
#else               // old CPU - use SSE 2
    __m128i tmp1 = _mm_mul_epu32(a.simd,b.simd); /* mul 2,0*/
    __m128i tmp2 = _mm_mul_epu32( _mm_srli_si128(a.simd,4), _mm_srli_si128(b.simd,4)); /* mul 3,1 */
    return VecS32(_mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0)))); /* shuffle results to [63..0] and pack */
#endif
}
inline VecS32 mulSSE4(const VecS32 &a, const VecS32 &b){
	return VecS32(_mm_mullo_epi32(a.simd, b.simd));
}
inline VecS32 operator |(const VecS32 &a, const VecS32 &b)		{ return VecS32(_mm_or_si128(a.simd, b.simd)); }
inline VecS32 operator &(const VecS32 &a, const VecS32 &b)		{ return VecS32(_mm_and_si128(a.simd, b.simd)); }


inline VecS32 andnot(const VecS32 &a, const VecS32 &b)			{ return VecS32(_mm_andnot_si128(a.simd, b.simd)); }

inline VecS32 vminSSE4(const VecS32 &a, const VecS32 &b)	{ return VecS32(_mm_min_epi32(a.simd, b.simd)); }
inline VecS32 vmaxSSE4(const VecS32 &a, const VecS32 &b)	{ return VecS32(_mm_max_epi32(a.simd, b.simd)); }
#ifdef __SSE4_1__
	inline VecS32 vmin(const VecS32 &a, const VecS32 &b)			{ return VecS32(_mm_min_epi32(a.simd, b.simd)); }
	inline VecS32 vmax(const VecS32 &a, const VecS32 &b)			{ return VecS32(_mm_max_epi32(a.simd, b.simd)); }
#else
	//SSE2

	// a -= b;
    // a &= a >> 31;
    // a += b;
	inline VecS32 vmin(const VecS32 &a, const VecS32 &b)	{ 
		__m128i aMinusB = _mm_sub_epi32(a.simd, b.simd);//a-b
		aMinusB = _mm_and_si128(aMinusB,_mm_srai_epi32(aMinusB,31)); // &= >> 31
		return VecS32(_mm_add_epi32(aMinusB,b.simd)); 
	}
	// a -= b;
	// a &= (~a) >> 31;
	// a += b;
	inline VecS32 vmax(const VecS32 &a, const VecS32 &b)	{ 
		__m128i aMinusB = _mm_sub_epi32(a.simd, b.simd);//a-b
		aMinusB = _mm_and_si128(aMinusB,_mm_srai_epi32(_mm_sub_epi32(_mm_setzero_si128(),aMinusB),31)); //&= ~ >> 31
		return VecS32(_mm_add_epi32(aMinusB,b.simd)); 
	}
#endif

// Functions not operator overloads because the semantics (returns mask)
// are very different from scalar comparison ops.
inline VecS32 cmplt(const VecS32 &a, const VecS32 &b)			{ return VecS32(_mm_cmplt_epi32(a.simd, b.simd)); }
inline VecS32 cmpgt(const VecS32 &a, const VecS32 &b)			{ return VecS32(_mm_cmpgt_epi32(a.simd, b.simd)); }

#ifdef __SSE4_1__
	inline bool is_all_zeros(const VecS32 &a)						{ return _mm_testz_si128(a.simd, a.simd) != 0; }
	inline bool is_all_negative(const VecS32 &a)					{ return _mm_testc_si128(_mm_set1_epi32(0x80000000), a.simd) != 0; }
#endif

// can't overload << since parameter to immed. shifts must be a compile-time constant
// and debug builds that don't inline can't deal with this
template<int N> inline VecS32 shiftl(const VecS32 &x)			{ return VecS32(_mm_slli_epi32(x.simd, N)); }

union VecF32 {
	float lane[4];
	__m128 simd;

	VecF32() {}
	explicit VecF32(float x) : simd(_mm_set1_ps(x)) {}
	explicit VecF32(const __m128 &in) : simd(in) {}
	VecF32(const VecF32 &x) : simd(x.simd) {}
	VecF32(float a, float b, float c, float d) : simd(_mm_setr_ps(a, b, c, d)) {}

	static VecF32 zero()							{ return VecF32(_mm_setzero_ps()); }
	static VecF32 load(const float *ptr)			{ return VecF32(_mm_load_ps(ptr)); }
	static VecF32 loadu(const float *ptr)			{ return VecF32(_mm_loadu_ps(ptr)); }
	void store(float *ptr)							{ _mm_store_ps(ptr, simd); }
	void storeu(float *ptr)							{ _mm_storeu_ps(ptr, simd); }

	VecF32 &operator =(const VecF32 &b)				{ simd = b.simd; return *this; }
	VecF32 &operator +=(const VecF32 &b)			{ simd = _mm_add_ps(simd, b.simd); return *this; }
	VecF32 &operator -=(const VecF32 &b)			{ simd = _mm_sub_ps(simd, b.simd); return *this; }
	VecF32 &operator *=(const VecF32 &b)			{ simd = _mm_mul_ps(simd, b.simd); return *this; }
	VecF32 &operator /=(const VecF32 &b)			{ simd = _mm_div_ps(simd, b.simd); return *this; }
};

inline VecF32 operator +(const VecF32 &a, const VecF32 &b)		{ return VecF32(_mm_add_ps(a.simd, b.simd)); }
inline VecF32 operator -(const VecF32 &a, const VecF32 &b)		{ return VecF32(_mm_sub_ps(a.simd, b.simd)); }
inline VecF32 operator *(const VecF32 &a, const VecF32 &b)		{ return VecF32(_mm_mul_ps(a.simd, b.simd)); }
inline VecF32 operator /(const VecF32 &a, const VecF32 &b)		{ return VecF32(_mm_div_ps(a.simd, b.simd)); }

// Functions not operator overloads because bitwise ops on floats should be explicit
inline VecF32 or(const VecF32 &a, const VecF32 &b)				{ return VecF32(_mm_or_ps(a.simd, b.simd)); }
inline VecF32 and(const VecF32 &a, const VecF32 &b)				{ return VecF32(_mm_and_ps(a.simd, b.simd)); }

inline VecF32 vmin(const VecF32 &a, const VecF32 &b)			{ return VecF32(_mm_min_ps(a.simd, b.simd)); }
inline VecF32 vmax(const VecF32 &a, const VecF32 &b)			{ return VecF32(_mm_max_ps(a.simd, b.simd)); }

// Functions not operator overloads because the semantics (returns mask)
// are very different from scalar comparison ops.
inline VecF32 cmplt(const VecF32 &a, const VecF32 &b)			{ return VecF32(_mm_cmplt_ps(a.simd, b.simd)); }
inline VecF32 cmple(const VecF32 &a, const VecF32 &b)			{ return VecF32(_mm_cmple_ps(a.simd, b.simd)); }
inline VecF32 cmpgt(const VecF32 &a, const VecF32 &b)			{ return VecF32(_mm_cmpgt_ps(a.simd, b.simd)); }
inline VecF32 cmpge(const VecF32 &a, const VecF32 &b)			{ return VecF32(_mm_cmpge_ps(a.simd, b.simd)); }

inline VecS32 ftoi(const VecF32 &x)								{ return VecS32(_mm_cvttps_epi32(x.simd)); }
inline VecS32 ftoi_round(const VecF32 &x)						{ return VecS32(_mm_cvtps_epi32(x.simd)); }
inline VecF32 itof(const VecS32 &x)								{ return VecF32(_mm_cvtepi32_ps(x.simd)); }

inline VecS32 float2bits(const VecF32 &x)						{ return VecS32(_mm_castps_si128(x.simd)); }
inline VecF32 bits2float(const VecS32 &x)						{ return VecF32(_mm_castsi128_ps(x.simd)); }

#ifdef __SSE4_1__
	// Select between a and b based on sign (MSB) of mask
	inline VecF32 select(const VecF32 &a, const VecF32 &b, const VecF32 &mask)	{ return VecF32(_mm_blendv_ps(a.simd, b.simd, mask.simd)); }
	// Select between a and b based on sign (MSB) of mask
	inline VecF32 select(const VecF32 &a, const VecF32 &b, const VecS32 &mask)	{ return VecF32(_mm_blendv_ps(a.simd, b.simd, _mm_castsi128_ps(mask.simd))); }
#else
	// Select between a and b based on sign (MSB) of mask
	inline VecF32 select(const VecF32 &a, const VecF32 &b, const VecS32 &mask)	{
		//Mask is in the form
		//0x8..._.... 0x8..._.... 0x8..._.... 0x8..._.... -> b.x,b.y,b.z,b.w
		//Map the mask from 0x8???_??? to 0x1 or 0x0
		auto fullMask = _mm_srli_epi32(mask.simd,31);
		//Get the complement of each component,mapping 0x1 to -1(0xFFFF_FFFF) and 0x0 to 0
		fullMask = _mm_sub_epi32(_mm_setzero_si128(),fullMask); 
		__m128 bitMask = _mm_castsi128_ps(fullMask);
		__m128 bb = _mm_and_ps(bitMask,b.simd);
		__m128 ba = _mm_andnot_ps(bitMask,a.simd);
		return VecF32(_mm_or_ps(ba,bb));
	}
#endif

struct VecF32Soa {
	VecF32 x;
	VecF32 y;
	VecF32 z;
	VecF32 w;
};
