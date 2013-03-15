#pragma once

inline vec4f::vec4f(vec2f v) : x(v.x),y(v.y)        { }
inline vec4f::vec4f(vec3f v) : x(v.x),y(v.y),z(v.z) { }
inline vec4f::vec4f(float xx,float yy,float zz,float ww) : x(xx),y(yy),z(zz),w(ww)  { }

inline vec3f vec4f::xyz() const { return vec3f(x,y,z); }
inline vec2f vec4f::xy() const  { return vec2f(x,y); }

inline float vec4f::length() const {
	return sqrtf(x*x+y*y+z*z+w*w);
}
inline float vec4f::lengthSquared() const {
	return x*x+y*y+z*z+w*w;
}
inline float vec4f::length3() const {
	return sqrtf(x*x+y*y+z*z);
}
inline float vec4f::length3Squared() const {
	return x*x+y*y+z*z;
}
inline vec4f vec4f::normalize() const { 
	float len = 1.0f/(sqrtf(x*x+y*y+z*z+w*w)); 
	return (*this) * len; 
}
inline float vec4f::dot(const vec4f& v) const  { return x*v.x+y*v.y+z*v.z+w*v.w; }
inline float vec4f::dot3(const vec4f& v) const { return x*v.x+y*v.y+z*v.z; }

#ifdef ARPHEG_ARCH_X86 //Assume SSE2 is supported

	inline vec4f::vec4f(float xx) {
		__m128 mk = _mm_set_ps1(xx);
		_mm_store_ps(&x,mk);
	}
	inline vec4f::vec4f(__m128 value) { _mm_store_ps(&x,value); }
	inline void vec4f::operator = (__m128 value) { _mm_store_ps(&x,value); }

	inline vec4f vec4f::operator + (const vec4f& v) const { return vec4f(_mm_add_ps(*(__m128*)(&x),*(__m128*)(&v.x))); } 
	inline vec4f vec4f::operator - (const vec4f& v) const { return vec4f(_mm_sub_ps(*(__m128*)(&x),*(__m128*)(&v.x))); } 
	inline vec4f vec4f::operator * (const vec4f& v) const { return vec4f(_mm_mul_ps(*(__m128*)(&x),*(__m128*)(&v.x))); }
	inline vec4f vec4f::operator / (const vec4f& v) const { return vec4f(_mm_div_ps(*(__m128*)(&x),*(__m128*)(&v.x))); }
	inline vec4f vec4f::operator * (float k) const { 
		__m128 mk = _mm_set_ps1(k);
		return vec4f(_mm_mul_ps(*(__m128*)(&x),mk));
	}

	inline vec4f vec4f::zero() { return vec4f(_mm_setzero_ps()); }
	inline vec4f vec4f::load(const float *ptr) { return vec4f(_mm_load_ps(ptr)); }
	inline vec4f vec4f::unalignedLoad(const float *ptr) { return vec4f(_mm_loadu_ps(ptr)); }
	inline void  vec4f::store(float *ptr) const { _mm_store_ps(ptr, *(__m128*)(&x)); }
	inline void  vec4f::unalignedStore(float *ptr) const { _mm_storeu_ps(ptr, *(__m128*)(&x)); }

	inline vec4f vec4f::min(const vec4f& x,const vec4f& y) {
		return vec4f(_mm_min_ps(*(__m128*)(&x.x),*(__m128*)(&y.x)));
	}
	inline vec4f vec4f::max(const vec4f& x,const vec4f& y) {
		return vec4f(_mm_max_ps(*(__m128*)(&x.x),*(__m128*)(&y.x)));
	}
	//TODO sse shuffle
	inline vec4f vec4f::xxxx() const { return vec4f(x,x,x,x); }
	inline vec4f vec4f::yyyy() const { return vec4f(y,y,y,y); }
	inline vec4f vec4f::zzzz() const { return vec4f(z,z,z,z); }
	inline vec4f vec4f::wwww() const { return vec4f(w,w,w,w); }

#else

	inline vec4f::vec4f(float xx) : x(xx),y(xx),z(xx),w(xx) { }

	inline vec4f vec4f::operator + (const vec4f& v) const { return vec4f(x+v.x,y+v.y,z+v.z,w+v.w); } 
	inline vec4f vec4f::operator - (const vec4f& v) const { return vec4f(x-v.x,y-v.y,z-v.z,w-v.w); } 
	inline vec4f vec4f::operator * (const vec4f& v) const { return vec4f(x*v.x,y*v.y,z*v.z,w*v.w); }
	inline vec4f vec4f::operator / (const vec4f& v) const { return vec4f(x/v.x,y/v.y,z/v.z,w/v.w); }
	inline vec4f operator * (float k) const { return vec4f(x*k,y*k,z*k,w*k); }

	inline vec4f vec4f::zero() { return vec4f(0.f,0.f,0.f,0.f); }
	inline vec4f vec4f::load(const float *ptr) { return vec4f(ptr[0],ptr[1],ptr[2],ptr[3]); }
	inline vec4f vec4f::unalignedLoad(const float *ptr) { return vec4f(ptr[0],ptr[1],ptr[2],ptr[3]); }
	inline void  vec4f::store(float *ptr) const { ptr[0] = x;ptr[1] = y;ptr[2] =z;ptr[3] =w; }
	inline void  vec4f::unalignedStore(float *ptr) const { ptr[0] = x;ptr[1] = y;ptr[2] =z;ptr[3] =w; }

	inline vec4f vec4f::min(const vec4f& x,const vec4f& y) {
		return vec4f(std::min(x.x,y.x),std::min(x.y,y.y),std::min(x.z,y.z),std::min(x.w,y.w));
	}
	inline vec4f vec4f::max(const vec4f& x,const vec4f& y) {
		return vec4f(std::max(x.x,y.x),std::max(x.y,y.y),std::max(x.z,y.z),std::max(x.w,y.w));
	}
	inline vec4f vec4f::xxxx() const { return vec4f(x,x,x,x); }
	inline vec4f vec4f::yyyy() const { return vec4f(y,y,y,y); }
	inline vec4f vec4f::zzzz() const { return vec4f(z,z,z,z); }
	inline vec4f vec4f::wwww() const { return vec4f(w,w,w,w); }

#endif

inline vec4f vec4f::lerp(const vec4f& a,const vec4f& b,float k){
	return (a*(1.0f-k)) + (b*k);
}
