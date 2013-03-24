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
#ifdef ARPHEG_ARCH_X86
	explicit inline vec4f(__m128 value);
	inline void operator = (__m128 value);
	inline __m128 m128() const;
#endif
	
	inline vec4f operator + (const vec4f& v) const;
	inline vec4f operator - (const vec4f& v) const;
	inline vec4f operator * (const vec4f& v) const;
	inline vec4f operator / (const vec4f& v) const;
	inline vec4f operator * (float k) const;

	inline float sum() const;
	inline float dot(const vec4f& v) const;
	inline float dot3(const vec4f& v) const;
	inline float length() const;
	inline float lengthSquared() const;
	inline float length3() const;
	inline float length3Squared() const;
	inline vec4f normalize() const;

	inline vec3f xyz() const;
	inline vec2f xy() const;
	inline vec4f xxxx() const;
	inline vec4f yyyy() const;
	inline vec4f zzzz() const;
	inline vec4f wwww() const;
	
	static inline vec4f zero();
	static inline vec4f load(const float *ptr);
	static inline vec4f unalignedLoad(const float *ptr);
	inline void store(float *ptr) const;
	inline void unalignedStore(float *ptr) const;

	static inline vec4f min(const vec4f& x,const vec4f& y);
	static inline vec4f max(const vec4f& x,const vec4f& y);
	static inline vec4f lerp(const vec4f& a,const vec4f& b,float k);
} STRUCT_POSTALIGN(16);
#include "math/vec4f.h"

STRUCT_PREALIGN(16) struct Quaternion {
	vec4f v_;

	inline Quaternion () {}
	inline Quaternion(float xx,float yy,float zz,float ww);
	explicit inline Quaternion(const vec4f& v);
#ifdef ARPHEG_ARCH_X86
	explicit inline Quaternion(__m128 value);
#endif

	static inline Quaternion identity();
	inline Quaternion operator* (const Quaternion& v) const;
	inline Quaternion operator* (float k) const;
	inline Quaternion operator+ (const Quaternion& v) const;
	inline Quaternion operator- (const Quaternion& v) const;
	inline vec4f v() const;
	inline float norm() const;
	inline Quaternion conjugate() const;
	
	static inline Quaternion rotateX(float theta);
	static inline Quaternion rotateY(float theta);
	static inline Quaternion rotateZ(float theta);
	static inline Quaternion rotate(vec3f axis,float theta);
	inline vec3f rotate(const vec3f& point) const;


	static inline Quaternion lerp(const Quaternion& a,const Quaternion& b,float k);
	static inline Quaternion slerp(const Quaternion& a,const Quaternion& b,float k);
} STRUCT_POSTALIGN(16);
#include "math/quaternion.h"

//The matrices are stored in Column-major(OpenGL order).
STRUCT_PREALIGN(16) struct mat34f {
	vec4f a,b,c; //columns

	inline mat34f() { }
	inline mat34f(const vec4f& aa,const vec4f& bb,const vec4f& cc) : a(aa),b(bb),c(cc) { }	
} STRUCT_POSTALIGN(16) ;

STRUCT_PREALIGN(16) struct mat44f {
	vec4f a,b,c,d; //columns

	inline mat44f() { }
	inline mat44f(const vec4f& aa,const vec4f& bb,const vec4f& cc,const vec4f& dd) : a(aa),b(bb),c(cc),d(dd) { }	
	explicit inline mat44f(const mat34f& mat);

	inline mat44f operator + (const mat44f& other) const;
	inline mat44f operator * (float k) const; 
	inline mat44f operator * (const mat44f& other) const; 
	inline void operator *= (const mat44f& other);
	
	inline void transposeSelf();

	static void multiply(const mat44f& m1,const mat44f& m2,mat44f& result);
	static void multiply(mat44f& m1,const mat44f& m2);

	static mat44f identity();
	static mat44f translate(vec3f x);
	static mat44f scale(vec3f x);
	static mat44f rotate(const Quaternion& rotation);
	static mat44f ortho(vec2f min,vec2f max,float near = -1.0f,float far = 1.0f);
	static mat44f perspective(float fovy, float aspect, float znear, float zfar);
	static mat44f lookAt(vec3f eye, vec3f center, vec3f up);

	//Assume: The direction is a vector of unit length 1.
	static mat44f translateRotateScale2D(vec2f translation,vec2f direction,vec2f scale);
	static mat44f translateRotateScale2D(const mat44f& projectionView,vec2f translation,vec2f direction,vec2f scale);

	static mat44f translateRotateScale(const vec3f& translation,const Quaternion& rotation,const vec3f& scaling);

} STRUCT_POSTALIGN(16) ;

inline  mat44f::mat44f(const mat34f& mat) : a(mat.a),b(mat.b),c(mat.c),d(0.f,0.f,0.f,1.f) {
}
inline mat44f mat44f::operator * (const mat44f& other) const {
	mat44f result;
	multiply(*this,other,result);
	return result;
}
inline void mat44f::operator *= (const mat44f& other) {
	multiply(*this,other);
}
inline mat44f mat44f::operator + (const mat44f& other) const {
	return mat44f(a+other.a,b+other.b,c+other.c,d+other.d);
}
inline mat44f mat44f::operator * (float k) const {
	return mat44f(a*k,b*k,c*k,d*k);
}
inline void mat44f::transposeSelf() {
#ifdef ARPHEG_ARCH_X86
	__m128 v0 = _mm_load_ps(&a.x);
	__m128 v1 = _mm_load_ps(&b.x);
	__m128 v2 = _mm_load_ps(&c.x);
	__m128 v3 = _mm_load_ps(&d.x);
	_MM_TRANSPOSE4_PS(v0, v1, v2, v3);
	a = vec4f(v0);b = vec4f(v1);c = vec4f(v2);d = vec4f(v3);
#else
	a = vec4f(a.x,b.x,c.x,d.x);
	b = vec4f(a.y,b.y,c.y,d.y);
	c = vec4f(a.z,b.z,c.z,d.z);
	d = vec4f(a.w,b.w,c.w,d.w);
#endif
}

inline vec4f operator * (const mat44f& m,const vec4f& v);
inline vec4f operator * (const mat44f& m,const vec4f& v) {
#ifdef PLATFORM_MATH_MAT_ROWMAJOR
	vec4f res;
	res.x = m.a.x * v.x + m.a.y * v.y + m.a.z * v.z + m.a.w * v.w;
	res.y = m.b.x * v.x + m.b.y * v.y + m.b.z * v.z + m.b.w * v.w;
	res.z = m.c.x * v.x + m.c.y * v.y + m.c.z * v.z + m.c.w * v.w;
	res.w = m.d.x * v.x + m.d.y * v.y + m.d.z * v.z + m.d.z * v.w;
	return res;
#else
#ifdef ARPHEG_ARCH_X86
	__m128 vx = _mm_set_ps1(v.x);
	vx = _mm_mul_ps(*(__m128*)(&m.a.x),vx);
	__m128 vy = _mm_set_ps1(v.y);
	vy = _mm_mul_ps(*(__m128*)(&m.b.x),vy);
	vx = _mm_add_ps(vx,vy);
	__m128 vz = _mm_set_ps1(v.z);
	vz = _mm_mul_ps(*(__m128*)(&m.c.x),vz);
	vx = _mm_add_ps(vx,vz);
	__m128 vw = _mm_set_ps1(v.w);
	vw = _mm_mul_ps(*(__m128*)(&m.d.x),vw);
	return vec4f(_mm_add_ps(vx,vw));
#else
	vec4f res;
	res.x = m.a.x * v.x + m.b.x * v.y + m.c.x * v.z + m.d.x * v.w;
	res.y = m.a.y * v.x + m.b.y * v.y + m.c.y * v.z + m.d.y * v.w;
	res.z = m.a.z * v.x + m.b.z * v.y + m.c.z * v.z + m.d.z * v.w;
	res.w = m.a.w * v.x + m.b.w * v.y + m.c.w * v.z + m.d.w * v.w;
	return res;
#endif
#endif
}


struct vec2i {
	int32 x,y;

	inline vec2i() {}
	inline vec2i(int32 xx,int32 yy) : x(xx),y(yy) {}
	inline vec2i operator + (vec2i v) const { return vec2i(x+v.x,y+v.y); } 
	inline vec2i operator - (vec2i v) const { return vec2i(x-v.x,y-v.y); } 
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

