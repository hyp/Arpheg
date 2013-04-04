#include "math.h"


void mat44f::multiply(const mat44f& m1,const mat44f& m2,mat44f& result){
#ifdef ARPHEG_ARCH_X86
	auto m1a = m1.a.m128();
	auto m1b = m1.b.m128();
	auto m1c = m1.c.m128();
	auto m1d = m1.d.m128();

	auto v2 = m2.a.m128();
	auto r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.a = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.b.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.b = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.c.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.c = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.d.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.d = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);
#else
	//First column
	result.a  = vec4f(
		m1.a.x*m2.a.x + m1.b.x*m2.a.y + m1.c.x*m2.a.z + m1.d.x*m2.a.w,
		m1.a.y*m2.a.x + m1.b.y*m2.a.y + m1.c.y*m2.a.z + m1.d.y*m2.a.w,
		m1.a.z*m2.a.x + m1.b.z*m2.a.y + m1.c.z*m2.a.z + m1.d.z*m2.a.w,
		m1.a.w*m2.a.x + m1.b.w*m2.a.y + m1.c.w*m2.a.z + m1.d.w*m2.a.w);
 
    // Second column
	result.b = vec4f(
		m1.a.x*m2.b.x + m1.b.x*m2.b.y + m1.c.x*m2.b.z + m1.d.x*m2.b.w,
		m1.a.y*m2.b.x + m1.b.y*m2.b.y + m1.c.y*m2.b.z + m1.d.y*m2.b.w,
		m1.a.z*m2.b.x + m1.b.z*m2.b.y + m1.c.z*m2.b.z + m1.d.z*m2.b.w,
		m1.a.w*m2.b.x + m1.b.w*m2.b.y + m1.c.w*m2.b.z + m1.d.w*m2.b.w);
 
    // Third column
	result.c = vec4f(
		m1.a.x*m2.c.x + m1.b.x*m2.c.y + m1.c.x*m2.c.z + m1.d.x*m2.c.w,
		m1.a.y*m2.c.x + m1.b.y*m2.c.y + m1.c.y*m2.c.z + m1.d.y*m2.c.w,
		m1.a.z*m2.c.x + m1.b.z*m2.c.y + m1.c.z*m2.c.z + m1.d.z*m2.c.w,
		m1.a.w*m2.c.x + m1.b.w*m2.c.y + m1.c.w*m2.c.z + m1.d.w*m2.c.w);
 
    // Fourth column
	result.d = vec4f(
		m1.a.x*m2.d.x + m1.b.x*m2.d.y + m1.c.x*m2.d.z + m1.d.x*m2.d.w,
		m1.a.y*m2.d.x + m1.b.y*m2.d.y + m1.c.y*m2.d.z + m1.d.y*m2.d.w,
		m1.a.z*m2.d.x + m1.b.z*m2.d.y + m1.c.z*m2.d.z + m1.d.z*m2.d.w,
		m1.a.w*m2.d.x + m1.b.w*m2.d.y + m1.c.w*m2.d.z + m1.d.w*m2.d.w);
#endif
}
void mat44f::multiply(mat44f& m1,const mat44f& m2){
#ifdef ARPHEG_ARCH_X86
	auto m1a = m1.a.m128();
	auto m1b = m1.b.m128();
	auto m1c = m1.c.m128();
	auto m1d = m1.d.m128();

	auto v2 = m2.a.m128();
	auto r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	m1.a =  _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));

	v2 = m2.b.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	m1.b = _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));

	v2 = m2.c.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	m1.c = _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));

	v2 = m2.d.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	m1.d = _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));
#else
	//First column
	auto ra  = vec4f(
		m1.a.x*m2.a.x + m1.b.x*m2.a.y + m1.c.x*m2.a.z + m1.d.x*m2.a.w,
		m1.a.y*m2.a.x + m1.b.y*m2.a.y + m1.c.y*m2.a.z + m1.d.y*m2.a.w,
		m1.a.z*m2.a.x + m1.b.z*m2.a.y + m1.c.z*m2.a.z + m1.d.z*m2.a.w,
		m1.a.w*m2.a.x + m1.b.w*m2.a.y + m1.c.w*m2.a.z + m1.d.w*m2.a.w);
 
    // Second column
	auto rb = vec4f(
		m1.a.x*m2.b.x + m1.b.x*m2.b.y + m1.c.x*m2.b.z + m1.d.x*m2.b.w,
		m1.a.y*m2.b.x + m1.b.y*m2.b.y + m1.c.y*m2.b.z + m1.d.y*m2.b.w,
		m1.a.z*m2.b.x + m1.b.z*m2.b.y + m1.c.z*m2.b.z + m1.d.z*m2.b.w,
		m1.a.w*m2.b.x + m1.b.w*m2.b.y + m1.c.w*m2.b.z + m1.d.w*m2.b.w);
 
    // Third column
	auto rc = vec4f(
		m1.a.x*m2.c.x + m1.b.x*m2.c.y + m1.c.x*m2.c.z + m1.d.x*m2.c.w,
		m1.a.y*m2.c.x + m1.b.y*m2.c.y + m1.c.y*m2.c.z + m1.d.y*m2.c.w,
		m1.a.z*m2.c.x + m1.b.z*m2.c.y + m1.c.z*m2.c.z + m1.d.z*m2.c.w,
		m1.a.w*m2.c.x + m1.b.w*m2.c.y + m1.c.w*m2.c.z + m1.d.w*m2.c.w);
 
    // Fourth column
	auto rd = vec4f(
		m1.a.x*m2.d.x + m1.b.x*m2.d.y + m1.c.x*m2.d.z + m1.d.x*m2.d.w,
		m1.a.y*m2.d.x + m1.b.y*m2.d.y + m1.c.y*m2.d.z + m1.d.y*m2.d.w,
		m1.a.z*m2.d.x + m1.b.z*m2.d.y + m1.c.z*m2.d.z + m1.d.z*m2.d.w,
		m1.a.w*m2.d.x + m1.b.w*m2.d.y + m1.c.w*m2.d.z + m1.d.w*m2.d.w);
	m1.a = ra;m1.b = rb;m1.c = rc;m1.d = rd;
#endif
}

void mat34fRowMajor::multiply(const mat34fRowMajor& m1,const mat34fRowMajor& m2,mat34fRowMajor& result){
#ifdef ARPHEG_ARCH_X86
	auto m1a = m1.a.m128();
	auto m1b = m1.b.m128();
	auto m1c = m1.c.m128();
	auto m1d = vec4f(0.0f,0.0f,0.0f,1.0f).m128();

	auto v2 = m2.a.m128();
	auto r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.a = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.b.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.b = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.c.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	result.c = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

#else
	//First row
	result.a  = vec4f(
		m1.a.x*m2.a.x + m1.b.x*m2.a.y + m1.c.x*m2.a.z ,
		m1.a.y*m2.a.x + m1.b.y*m2.a.y + m1.c.y*m2.a.z ,
		m1.a.z*m2.a.x + m1.b.z*m2.a.y + m1.c.z*m2.a.z ,
		m1.a.w*m2.a.x + m1.b.w*m2.a.y + m1.c.w*m2.a.z + m2.a.w);
 
    // Second row
	result.b = vec4f(
		m1.a.x*m2.b.x + m1.b.x*m2.b.y + m1.c.x*m2.b.z ,
		m1.a.y*m2.b.x + m1.b.y*m2.b.y + m1.c.y*m2.b.z ,
		m1.a.z*m2.b.x + m1.b.z*m2.b.y + m1.c.z*m2.b.z ,
		m1.a.w*m2.b.x + m1.b.w*m2.b.y + m1.c.w*m2.b.z + m2.b.w);
 
    // Third row
	result.c = vec4f(
		m1.a.x*m2.c.x + m1.b.x*m2.c.y + m1.c.x*m2.c.z ,
		m1.a.y*m2.c.x + m1.b.y*m2.c.y + m1.c.y*m2.c.z ,
		m1.a.z*m2.c.x + m1.b.z*m2.c.y + m1.c.z*m2.c.z ,
		m1.a.w*m2.c.x + m1.b.w*m2.c.y + m1.c.w*m2.c.z + m2.c.w);
#endif
}
void mat34fRowMajor::multiply(mat34fRowMajor& m1,const mat34fRowMajor& m2){
#ifdef ARPHEG_ARCH_X86
	auto m1a = m1.a.m128();
	auto m1b = m1.b.m128();
	auto m1c = m1.c.m128();
	auto m1d = vec4f(0.0f,0.0f,0.0f,1.0f).m128();

	auto v2 = m2.a.m128();
	auto r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	m1.a = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.b.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	m1.b = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);

	v2 = m2.c.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = vec4f::fma(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1)),r);
	r = vec4f::fma(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2)),r);
	m1.c = vec4f::fma(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3)),r);
#else
	// First row
	auto ra  = vec4f(
		m1.a.x*m2.a.x + m1.b.x*m2.a.y + m1.c.x*m2.a.z ,
		m1.a.y*m2.a.x + m1.b.y*m2.a.y + m1.c.y*m2.a.z ,
		m1.a.z*m2.a.x + m1.b.z*m2.a.y + m1.c.z*m2.a.z ,
		m1.a.w*m2.a.x + m1.b.w*m2.a.y + m1.c.w*m2.a.z + m2.a.w);
 
    // Second row
	auto rb = vec4f(
		m1.a.x*m2.b.x + m1.b.x*m2.b.y + m1.c.x*m2.b.z ,
		m1.a.y*m2.b.x + m1.b.y*m2.b.y + m1.c.y*m2.b.z ,
		m1.a.z*m2.b.x + m1.b.z*m2.b.y + m1.c.z*m2.b.z ,
		m1.a.w*m2.b.x + m1.b.w*m2.b.y + m1.c.w*m2.b.z + m2.b.w);
 
    // Third row
	auto rc = vec4f(
		m1.a.x*m2.c.x + m1.b.x*m2.c.y + m1.c.x*m2.c.z ,
		m1.a.y*m2.c.x + m1.b.y*m2.c.y + m1.c.y*m2.c.z ,
		m1.a.z*m2.c.x + m1.b.z*m2.c.y + m1.c.z*m2.c.z ,
		m1.a.w*m2.c.x + m1.b.w*m2.c.y + m1.c.w*m2.c.z + m2.c.w);
 
	m1.a = ra;m1.b = rb;m1.c = rc;
#endif
}
	

#ifdef ARPHEG_ARCH_X86
// http://devmaster.net/forums/topic/11799-sse-mat4-inverse/

inline __m128 _mm_dot_ps(__m128 v1, __m128 v2) {
        __m128 mul0 = _mm_mul_ps(v1, v2);
        __m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
        __m128 add0 = _mm_add_ps(mul0, swp0);
        __m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(0, 1, 2, 3));
        __m128 add1 = _mm_add_ps(add0, swp1);
        return add1;
}

static inline void inverseSSE2(__m128 in[4],__m128 out[4])  {
    __m128 Fac0;
    {
            //      valType SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
            //      valType SubFactor00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
            //      valType SubFactor06 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
            //      valType SubFactor13 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

            __m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));

            __m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));
            __m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

            __m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
            __m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
            Fac0 = _mm_sub_ps(Mul00, Mul01);

            bool stop = true;
    }

    __m128 Fac1;
    {
            //      valType SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
            //      valType SubFactor01 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
            //      valType SubFactor07 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
            //      valType SubFactor14 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

            __m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));

            __m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));
            __m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

            __m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
            __m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
            Fac1 = _mm_sub_ps(Mul00, Mul01);

            bool stop = true;
    }


    __m128 Fac2;
    {
            //      valType SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
            //      valType SubFactor02 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
            //      valType SubFactor08 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
            //      valType SubFactor15 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

            __m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));
            __m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));

            __m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));
            __m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));

            __m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
            __m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
            Fac2 = _mm_sub_ps(Mul00, Mul01);

            bool stop = true;
    }

    __m128 Fac3;
    {
            //      valType SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
            //      valType SubFactor03 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
            //      valType SubFactor09 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
            //      valType SubFactor16 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

            __m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(3, 3, 3, 3));
            __m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

            __m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
            __m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(3, 3, 3, 3));

            __m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
            __m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
            Fac3 = _mm_sub_ps(Mul00, Mul01);

            bool stop = true;
    }

    __m128 Fac4;
    {
            //      valType SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
            //      valType SubFactor04 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
            //      valType SubFactor10 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
            //      valType SubFactor17 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

            __m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(2, 2, 2, 2));
            __m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

            __m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
            __m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(2, 2, 2, 2));

            __m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
            __m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
            Fac4 = _mm_sub_ps(Mul00, Mul01);

            bool stop = true;
    }

    __m128 Fac5;
    {
            //      valType SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
            //      valType SubFactor05 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
            //      valType SubFactor12 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
            //      valType SubFactor18 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

            __m128 Swp0a = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(1, 1, 1, 1));
            __m128 Swp0b = _mm_shuffle_ps(in[3], in[2], _MM_SHUFFLE(0, 0, 0, 0));

            __m128 Swp00 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(0, 0, 0, 0));
            __m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
            __m128 Swp03 = _mm_shuffle_ps(in[2], in[1], _MM_SHUFFLE(1, 1, 1, 1));

            __m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
            __m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
            Fac5 = _mm_sub_ps(Mul00, Mul01);

            bool stop = true;
    }

    __m128 SignA = _mm_set_ps( 1.0f,-1.0f, 1.0f,-1.0f);
    __m128 SignB = _mm_set_ps(-1.0f, 1.0f,-1.0f, 1.0f);

    // m[1][0]
    // m[0][0]
    // m[0][0]
    // m[0][0]
    __m128 Temp0 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(0, 0, 0, 0));
    __m128 Vec0 = _mm_shuffle_ps(Temp0, Temp0, _MM_SHUFFLE(2, 2, 2, 0));

    // m[1][1]
    // m[0][1]
    // m[0][1]
    // m[0][1]
    __m128 Temp1 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(1, 1, 1, 1));
    __m128 Vec1 = _mm_shuffle_ps(Temp1, Temp1, _MM_SHUFFLE(2, 2, 2, 0));

    // m[1][2]
    // m[0][2]
    // m[0][2]
    // m[0][2]
    __m128 Temp2 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(2, 2, 2, 2));
    __m128 Vec2 = _mm_shuffle_ps(Temp2, Temp2, _MM_SHUFFLE(2, 2, 2, 0));

    // m[1][3]
    // m[0][3]
    // m[0][3]
    // m[0][3]
    __m128 Temp3 = _mm_shuffle_ps(in[1], in[0], _MM_SHUFFLE(3, 3, 3, 3));
    __m128 Vec3 = _mm_shuffle_ps(Temp3, Temp3, _MM_SHUFFLE(2, 2, 2, 0));

    // col0
    // + (Vec1[0] * Fac0[0] - Vec2[0] * Fac1[0] + Vec3[0] * Fac2[0]),
    // - (Vec1[1] * Fac0[1] - Vec2[1] * Fac1[1] + Vec3[1] * Fac2[1]),
    // + (Vec1[2] * Fac0[2] - Vec2[2] * Fac1[2] + Vec3[2] * Fac2[2]),
    // - (Vec1[3] * Fac0[3] - Vec2[3] * Fac1[3] + Vec3[3] * Fac2[3]),
    __m128 Mul00 = _mm_mul_ps(Vec1, Fac0);
    __m128 Mul01 = _mm_mul_ps(Vec2, Fac1);
    __m128 Mul02 = _mm_mul_ps(Vec3, Fac2);
    __m128 Sub00 = _mm_sub_ps(Mul00, Mul01);
    __m128 Add00 = _mm_add_ps(Sub00, Mul02);
    __m128 Inv0 = _mm_mul_ps(SignB, Add00);

    // col1
    // - (Vec0[0] * Fac0[0] - Vec2[0] * Fac3[0] + Vec3[0] * Fac4[0]),
    // + (Vec0[0] * Fac0[1] - Vec2[1] * Fac3[1] + Vec3[1] * Fac4[1]),
    // - (Vec0[0] * Fac0[2] - Vec2[2] * Fac3[2] + Vec3[2] * Fac4[2]),
    // + (Vec0[0] * Fac0[3] - Vec2[3] * Fac3[3] + Vec3[3] * Fac4[3]),
    __m128 Mul03 = _mm_mul_ps(Vec0, Fac0);
    __m128 Mul04 = _mm_mul_ps(Vec2, Fac3);
    __m128 Mul05 = _mm_mul_ps(Vec3, Fac4);
    __m128 Sub01 = _mm_sub_ps(Mul03, Mul04);
    __m128 Add01 = _mm_add_ps(Sub01, Mul05);
    __m128 Inv1 = _mm_mul_ps(SignA, Add01);

    // col2
    // + (Vec0[0] * Fac1[0] - Vec1[0] * Fac3[0] + Vec3[0] * Fac5[0]),
    // - (Vec0[0] * Fac1[1] - Vec1[1] * Fac3[1] + Vec3[1] * Fac5[1]),
    // + (Vec0[0] * Fac1[2] - Vec1[2] * Fac3[2] + Vec3[2] * Fac5[2]),
    // - (Vec0[0] * Fac1[3] - Vec1[3] * Fac3[3] + Vec3[3] * Fac5[3]),
    __m128 Mul06 = _mm_mul_ps(Vec0, Fac1);
    __m128 Mul07 = _mm_mul_ps(Vec1, Fac3);
    __m128 Mul08 = _mm_mul_ps(Vec3, Fac5);
    __m128 Sub02 = _mm_sub_ps(Mul06, Mul07);
    __m128 Add02 = _mm_add_ps(Sub02, Mul08);
    __m128 Inv2 = _mm_mul_ps(SignB, Add02);

    // col3
    // - (Vec1[0] * Fac2[0] - Vec1[0] * Fac4[0] + Vec2[0] * Fac5[0]),
    // + (Vec1[0] * Fac2[1] - Vec1[1] * Fac4[1] + Vec2[1] * Fac5[1]),
    // - (Vec1[0] * Fac2[2] - Vec1[2] * Fac4[2] + Vec2[2] * Fac5[2]),
    // + (Vec1[0] * Fac2[3] - Vec1[3] * Fac4[3] + Vec2[3] * Fac5[3]));
    __m128 Mul09 = _mm_mul_ps(Vec0, Fac2);
    __m128 Mul10 = _mm_mul_ps(Vec1, Fac4);
    __m128 Mul11 = _mm_mul_ps(Vec2, Fac5);
    __m128 Sub03 = _mm_sub_ps(Mul09, Mul10);
    __m128 Add03 = _mm_add_ps(Sub03, Mul11);
    __m128 Inv3 = _mm_mul_ps(SignA, Add03);

    __m128 Row0 = _mm_shuffle_ps(Inv0, Inv1, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 Row1 = _mm_shuffle_ps(Inv2, Inv3, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 Row2 = _mm_shuffle_ps(Row0, Row1, _MM_SHUFFLE(2, 0, 2, 0));

    //      valType Determinant = m[0][0] * Inverse[0][0] 
    //                                              + m[0][1] * Inverse[1][0] 
    //                                              + m[0][2] * Inverse[2][0] 
    //                                              + m[0][3] * Inverse[3][0];
    __m128 Det0 = _mm_dot_ps(in[0], Row2);
    __m128 Rcp0 = _mm_div_ps(_mm_set_ps1(1.0f), Det0);
    //__m128 Rcp0 = _mm_rcp_ps(Det0);

    //      Inverse /= Determinant;
    out[0] = _mm_mul_ps(Inv0, Rcp0);
    out[1] = _mm_mul_ps(Inv1, Rcp0);
    out[2] = _mm_mul_ps(Inv2, Rcp0);
    out[3] = _mm_mul_ps(Inv3, Rcp0);
}
#endif

mat44f mat44f::inverse() const {
	mat44f result;
	inverseSSE2((__m128*)(&a.x),(__m128*)(&result.a.x));
	return result;
}


mat44f mat44f::identity() {
	return mat44f(vec4f(1.0f,0.0f,0.0f,0.0f), vec4f(0.0f,1.0f,0.0f,0.0f), vec4f(0.0f,0.0f,1.0f,0.0f), vec4f(0.0f,0.0f,0.0f,1.0f) );
}
mat34fRowMajor mat34fRowMajor::identity() {
	return mat34fRowMajor(vec4f(1.0f,0.0f,0.0f,0.0f),vec4f(0.0f,1.0f,0.0f,0.0f),vec4f(0.0f,0.0f,1.0f,0.0f));
}
mat44f mat44f::translate(vec3f x) {
	return mat44f(
		vec4f(1.0f,0.0f,0.0,0.0),
		vec4f(0.0f,1.0f,0.0,0.0),
		vec4f(0.0,0.0,1.0,0.0),
		vec4f(x.x,x.y,x.z,1.0f) );
}
mat44f mat44f::scale(vec3f x) {
	return mat44f(
		vec4f(x.x,0.0f,0.0,0.0),
		vec4f(0.0f,x.y,0.0,0.0),
		vec4f(0.0,0.0,x.z,0.0),
		vec4f(0.0,0.0,0.0,1.0f) );
}
#ifdef ARPHEG_ARCH_X86
#define QUAT_TO_MAT \
	auto v = rotation.v().m128();\
	auto temp = _mm_mul_ps(v,_mm_shuffle_ps(v,v,_MM_SHUFFLE(2,2,1,0)));\
	auto xxyyzzzw2 = vec4f( _mm_add_ps(temp,temp) ) ; \
	temp = _mm_mul_ps(v,_mm_shuffle_ps(v,v,_MM_SHUFFLE(1,1,0,2)));\
	auto xzxyyzyw2 = vec4f( _mm_add_ps(temp,temp) ); \
	auto xw2 = vec4f( _mm_mul_ss(v, _mm_shuffle_ps(v,v,_MM_SHUFFLE(3,3,3,3))) ).x * 2.0f;
#else
#define QUAT_TO_MAT \
	auto v = rotation.v();\
	auto temp = v * vec4f(v.x,v.y,v.z,v.z);\
	auto xxyyzzzw2 = temp + temp;\
	temp = v * vec4f(v.z,v.x,v.y,v.y);\
	auto xzxyyzyw2 = temp + temp;\
	auto xw2 = v.x * v.w * 2.0f;
#endif
mat44f mat44f::rotate(const Quaternion& rotation) {
	//Quaternion to matrix.
	QUAT_TO_MAT

	return mat44f(
		vec4f(1.0f - (xxyyzzzw2.y + xxyyzzzw2.z),(xzxyyzyw2.y + xxyyzzzw2.w),(xzxyyzyw2.x - xzxyyzyw2.w),0.0f),
		vec4f(xzxyyzyw2.y - xxyyzzzw2.w,1.0f - (xxyyzzzw2.x + xxyyzzzw2.z),(xzxyyzyw2.z + xw2),0.0f),
		vec4f(xzxyyzyw2.x + xzxyyzyw2.w,xzxyyzyw2.z - xw2,1.0f - (xxyyzzzw2.x + xxyyzzzw2.y)  ,0.0f),
		vec4f(0.0f,0.0f,0.0f,1.0f) );
}

mat34fRowMajor mat34fRowMajor::translate(vec3f x){
	return mat34fRowMajor(vec4f(1.0f,0.0f,0.0f,x.x),
		vec4f(0.0f,1.0f,0.0f,x.y),
		vec4f(0.0f,0.0f,1.0f,x.z));
}
mat34fRowMajor mat34fRowMajor::scale(vec3f x){
	return mat34fRowMajor(vec4f(x.x,0.0f,0.0f,0.f),
		vec4f(0.0f,x.y,0.0f,0.f),
		vec4f(0.0f,0.0f,x.z,0.f));
}
mat34fRowMajor mat34fRowMajor::rotate(const Quaternion& rotation){
	QUAT_TO_MAT

	return mat34fRowMajor(
		vec4f(1.0f - (xxyyzzzw2.y + xxyyzzzw2.z),xzxyyzyw2.y - xxyyzzzw2.w,xzxyyzyw2.x + xzxyyzyw2.w,0.0f),
		vec4f((xzxyyzyw2.y + xxyyzzzw2.w),1.0f - (xxyyzzzw2.x + xxyyzzzw2.z),xzxyyzyw2.z - xw2     ,0.0f),
		vec4f((xzxyyzyw2.x - xzxyyzyw2.w),(xzxyyzyw2.z + xw2),1.0f - (xxyyzzzw2.x + xxyyzzzw2.y)  ,0.0f));
}
mat34fRowMajor mat34fRowMajor::translateRotateScale(const vec3f& translation,const Quaternion& rotation,const vec3f& scaling){
	auto result = rotate(rotation);
	multiply(result,mat34fRowMajor(
		vec4f(scaling.x,0.f,0.f,translation.x),
		vec4f(0.f,scaling.y,0.f,translation.y),
		vec4f(0.f,0.f,scaling.z,translation.z)));
	return result;
}

mat44f mat44f::translateRotateScale2D(vec2f translation,vec2f direction,vec2f scale) {
	//Direction x,y = cos a, sin a (unit circle)
#ifdef PLATFORM_MATH_MAT_ROWMAJOR
	return mat44f(
		vec4f(scale.x * direction.x,scale.x * direction.y,0.0,translation.x),
		vec4f(scale.y * -direction.y,scale.y * direction.x,0.0,translation.y),
		vec4f(0.0,0.0,1.0,0.0),
		vec4f(0.0f,0.0f,0.0f,1.0f) );
#else
	return mat44f(
		vec4f(scale.x * direction.x,scale.x * direction.y,0.0,0.0),
		vec4f(scale.y * -direction.y,scale.y * direction.x,0.0,0.0),
		vec4f(0.0,0.0,1.0,0.0),
		vec4f(translation.x,translation.y,0.0f,1.0f) );
#endif
}
mat44f mat44f::translateRotateScale2D(const mat44f& projectionView,vec2f translation,vec2f direction,vec2f scale) {
	mat44f model,result;
	model = translateRotateScale2D(translation,direction,scale);
	multiply(projectionView,model,result);
	return result;
}
mat44f mat44f::translateRotateScale(const vec3f& translation,const Quaternion& rotation,const vec3f& scaling) {
	mat44f result = mat44f(
		vec4f(scaling.x,0.f,0.f,0.f),
		vec4f(0.f,scaling.y,0.f,0.f),
		vec4f(0.f,0.f,scaling.z,0.f),
		vec4f(translation.x,translation.y,translation.z,1.f));
	multiply(result,rotate(rotation));
	return result;
}

#undef QUAT_TO_MAT

//Special view + projection matrices.
mat44f mat44f::ortho(vec2f min,vec2f max,float near,float far) {
	const float right = max.x, left = min.x, top = min.y, bottom = max.y;
	float tx = - (right + left) / (right - left);
	float ty = - (top + bottom) / (top - bottom);
	float tz = - (far + near)   / (far - near);

	return mat44f(
		vec4f(2.0f/(right - left),0.0,0.0,0.0),
		vec4f(0.0,2.0f/(top - bottom),0.0,0.0),
		vec4f(0.0,0.0,(-2.0f)/(far-near),0.0),
		vec4f(tx,ty,tz,1.0f) );
}
mat44f mat44f::perspective(float fovy, float aspect, float znear, float zfar) {
    auto f = 1 / tanf(fovy / 2),
            A = (zfar + znear) / (znear - zfar),
            B = (2 * zfar * znear) / (znear - zfar);

    return mat44f (
			vec4f(f / aspect, 0, 0, 0),
            vec4f(0, f, 0, 0),
            vec4f(0, 0, A, -1),
            vec4f(0, 0, B, 0));
}
mat44f mat44f::lookAt(vec3f eye, vec3f center, vec3f up) {
	vec3f forward = (center - eye).normalize();
	vec3f side = forward.cross(up).normalize();
	up = side.cross(forward);

	mat44f result = identity();
	float* m = &result.a.x;
	m[0] = side.x;
	m[4] = side.y;
	m[8] = side.z;
	m[1] = up.x;
	m[5] = up.y;
	m[9] = up.z;
	m[2] = -forward.x;
	m[6] = -forward.y;
	m[10] = -forward.z;

	return result*mat44f::translate(eye*-1.0f);
}
