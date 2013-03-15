#pragma once

inline Quaternion::Quaternion(float xx,float yy,float zz,float ww) : x(xx),y(yy),z(zz),w(ww)  { }
inline Quaternion::Quaternion(const vec4f& v) : vec4f(v) {}
inline Quaternion Quaternion::identity() {
	return Quaternion(0.f,0.f,0.f,1.f);
}
inline float Quaternion::norm() const {
	return lengthSquared();
}
inline Quaternion Quaternion::operator* (const Quaternion& v) const {
	return v;//TODO
}
inline Quaternion Quaternion::operator* (float k) const {
	return Quaternion(vec4f::operator*(k));
}
inline vec4f Quaternion::v() const {
	return *this;
}

#ifdef ARPHEG_ARCH_X86 //Assume SSE2 is supported
	
	inline Quaternion::Quaternion(__m128 value) { _mm_store_ps(&x,value); }
	inline Quaternion Quaternion::conjugate() const {
		return Quaternion(-x,-y,-z,w);
	}

#else

#endif

inline Quaternion Quaternion::lerp(const Quaternion& a,const Quaternion& b,float k) {
	return Quaternion( (a*(1.0f - k)) + (b*k) );
}
inline vec3f Quaternion::rotate(const vec3f& point) const {
	vec3f uv,uuv;
	vec3f qvec(x,y,z);
	uv = qvec.cross(point);
	uuv = qvec.cross(uv);
	uv = uv*(2.0f*w);
	uuv = uuv*2.0f;
	return point+uv+uuv;
}
