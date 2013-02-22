#pragma once

#include "types.h"
#include "structLayout.h"
#include <algorithm>
#include <math.h>

//Intrinsics
#ifdef ARPHEG_ARCH_X86
	#ifdef _MSC_VER
		#include <intrin.h>
	#else
		#include <x86intrin.h> 
	#endif
#endif

namespace math {

	static const double pi = 3.141592653589793238462643;
	static const double e  = 2.718281828458563411277850;

	template<typename T>
	static inline T toRadians(T degrees) { return degrees * (T(pi) / T(180)); }
	template<typename T>
	static inline T toDegrees(T radians) { return radians * (T(180) / T(pi)); }
	template<typename T>
	static inline T lerp(T a,T b,T value) { return a + value * (b - a); }
	template<typename T,typename S>
	static inline T lerp(T a,T b,S value) { return a + (b - a) * value; }
	template<typename T>
	static inline T clamp(T x,T min,T max) { return std::min(max,std::max(min,x)); }
}

struct vec2f {
	float x,y;

	inline vec2f() {}
	inline vec2f(float xx,float yy) : x(xx),y(yy) {}
	inline vec2f operator - () const { return vec2f(-x,-y); } 
	inline vec2f operator * (float k) const { return vec2f(x*k,y*k); } 
	inline vec2f operator + (vec2f v) const { return vec2f(x+v.x,y+v.y); } 
	inline vec2f operator - (vec2f v) const { return vec2f(x-v.x,y-v.y); } 
	inline float lengthSquared() const { return x*x+y*y; }
	inline float length() const { return sqrtf(x*x+y*y); }
	inline vec2f perpendicular() const { return vec2f(-y,x); }
	inline vec2f normalize() const { float len = 1.0f/length(); return vec2f(x*len,y*len); }

	inline float dot(vec2f v) { return x*v.x+y*v.y; }

	static inline vec2f directionFromRadians(float th) { return vec2f(cosf(th),sinf(th)); }
	static inline vec2f directionFromDegrees(float th) { return directionFromRadians(math::toRadians(th)); }
};

struct vec3f {
	float x,y,z;

	inline vec3f(){}
	inline vec3f(float xx,float yy,float zz) : x(xx),y(yy),z(zz) {}

	inline vec3f operator - (const vec3f& v) const { return vec3f(x-v.x,y-v.y,z-v.z); } 
	inline vec3f operator + (const vec3f& v) const { return vec3f(x+v.x,y+v.y,z+v.z); } 
	inline vec3f operator * (float k) const { return vec3f(x*k,y*k,z*k); }

	inline float lengthSquared() const { return x*x+y*y+z*z; }
	inline float length() const { return sqrtf(x*x+y*y+z*z); }
	inline vec3f normalize() const { float len = 1.0f/length(); return vec3f(x*len,y*len,z*len); }
	inline float dot(vec3f v) { return x*v.x+y*v.y+z*v.z; }
	inline vec3f cross(vec3f v) const {
		return vec3f(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
	}
};

STRUCT_PREALIGN(16) struct vec4f {
	float x,y,z,w;

	inline vec4f() {}
	inline vec4f(float xx);
	inline vec4f(vec2f v);
	inline vec4f(vec3f v);
	inline vec4f(float xx,float yy,float zz,float ww);
	inline vec4f(__m128 value) { _mm_store_ps(&x,value); }

	inline vec4f operator + (const vec4f& v) const;
	inline vec4f operator - (const vec4f& v) const;
	inline vec4f operator * (const vec4f& v) const;
	inline vec4f operator / (const vec4f& v) const;
	inline vec4f operator * (float k) const;

	inline vec4f normalize() const { float len = 1.0f/(x*x+y*y+z*z+w*w); return vec4f(x*len,y*len,z*len,w*len); }
	inline float dot(const vec4f& v) const  { return x*v.x+y*v.y+z*v.z+w*v.w; }
	inline float dot3(const vec4f& v) const { return x*v.x+y*v.y+z*v.z; }

	static inline vec4f min(const vec4f& x,const vec4f& y);
	static inline vec4f max(const vec4f& x,const vec4f& y);
} STRUCT_POSTALIGN(16);

inline vec4f::vec4f(vec2f v) : x(v.x),y(v.y)        { }
inline vec4f::vec4f(vec3f v) : x(v.x),y(v.y),z(v.z) { }
inline vec4f::vec4f(float xx,float yy,float zz,float ww) : x(xx),y(yy),z(zz),w(ww)  { }

#ifdef ARPHEG_ARCH_X86

inline vec4f::vec4f(float xx) {
	__m128 mk = _mm_set_ps1(xx);
	_mm_store_ps(&x,mk);
}

inline vec4f vec4f::operator + (const vec4f& v) const { return vec4f(_mm_add_ps(*(__m128*)(&x),*(__m128*)(&v.x))); } 
inline vec4f vec4f::operator - (const vec4f& v) const { return vec4f(_mm_sub_ps(*(__m128*)(&x),*(__m128*)(&v.x))); } 
inline vec4f vec4f::operator * (const vec4f& v) const { return vec4f(_mm_mul_ps(*(__m128*)(&x),*(__m128*)(&v.x))); }
inline vec4f vec4f::operator / (const vec4f& v) const { return vec4f(_mm_div_ps(*(__m128*)(&x),*(__m128*)(&v.x))); }
inline vec4f vec4f::operator * (float k) const { 
	__m128 mk = _mm_set_ps1(k);
	return vec4f(_mm_mul_ps(*(__m128*)(&x),mk));
}
inline vec4f vec4f::min(const vec4f& x,const vec4f& y) {
	return vec4f(_mm_min_ps(*(__m128*)(&x.x),*(__m128*)(&y.x)));
}
inline vec4f vec4f::max(const vec4f& x,const vec4f& y) {
	return vec4f(_mm_max_ps(*(__m128*)(&x.x),*(__m128*)(&y.x)));
}

#else

inline vec4f::vec4f(float xx) : x(xx),y(xx),z(xx),w(xx) { }

inline vec4f vec4f::operator + (const vec4f& v) const { return vec4f(x+v.x,y+v.y,z+v.z,w+v.w); } 
inline vec4f vec4f::operator - (const vec4f& v) const { return vec4f(x-v.x,y-v.y,z-v.z,w-v.w); } 
inline vec4f vec4f::operator * (const vec4f& v) const { return vec4f(x*v.x,y*v.y,z*v.z,w*v.w); }
inline vec4f vec4f::operator / (const vec4f& v) const { return vec4f(x/v.x,y/v.y,z/v.z,w/v.w); }
inline vec4f operator * (float k) const { return vec4f(x*k,y*k,z*k,w*k); }

inline vec4f vec4f::min(const vec4f& x,const vec4f& y) {
	return vec4f(std::min(x.x,y.x),std::min(x.y,y.y),std::min(x.z,y.z),std::min(x.w,y.w));
}
inline vec4f vec4f::max(const vec4f& x,const vec4f& y) {
	return vec4f(std::max(x.x,y.x),std::max(x.y,y.y),std::max(x.z,y.z),std::max(x.w,y.w));
}

#endif

STRUCT_PREALIGN(16) struct mat44f {
	vec4f a,b,c,d; //columns

	inline mat44f() { }
	inline mat44f(const vec4f& aa,const vec4f& bb,const vec4f& cc,const vec4f& dd) : a(aa),b(bb),c(cc),d(dd) { }	
	mat44f operator * (const mat44f& other) const; 

	static mat44f identity();
	static mat44f ortho(vec2f min,vec2f max,float near = -1.0f,float far = 1.0f);
	static mat44f perspective(float fovy, float aspect, float znear, float zfar);
	static mat44f lookAt(vec3f eye, vec3f center, vec3f up);
	static mat44f translate(vec3f x);
	static mat44f scale(vec3f x);

	//Assume: The direction is a vector of unit length 1.
	static mat44f translateRotateScale2D(vec2f translation,vec2f direction,vec2f scale);
	static mat44f translateRotateScale2D(const mat44f& projectionView,vec2f translation,vec2f direction,vec2f scale);

	//
	
} STRUCT_POSTALIGN(16) ;

struct vec2i {
	int32 x,y;

	inline vec2i() {}
	inline vec2i(int32 xx,int32 yy) : x(xx),y(yy) {}
};

STRUCT_PREALIGN(16) struct vec4i {
	int32 x,y,z,w;

	inline vec4i() {}
	inline vec4i(int32 xx,int32 yy,int32 zz,int32 ww) : x(xx),y(yy),z(zz),w(ww) {}
	inline vec4i(const vec4f& v) : x(int32(v.x)),y(int32(v.y)),z(int32(v.z)),w(int32(v.w)) {}

	inline vec4i operator +(const vec4i& v) { return vec4i(x+v.x,y+v.y,z+v.z,w+v.w); }
	inline vec4i operator -(const vec4i& v) { return vec4i(x-v.x,y-v.y,z-v.z,w-v.w); }
} STRUCT_POSTALIGN(16);


namespace math {
	inline float distance(vec2f a,vec2f b) { return sqrtf((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y)); }
	inline float distanceSquared(vec2f a,vec2f b) { return (a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y); }
}

struct AABB {
	vec2f min, max;

	inline AABB() {}
	inline AABB(vec2f mmin,vec2f mmax) : min(mmin),max(mmax) {}
};

