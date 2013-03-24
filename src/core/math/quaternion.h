#pragma once

inline Quaternion::Quaternion(float xx,float yy,float zz,float ww) : v_(xx,yy,zz,ww) { }
inline Quaternion::Quaternion(const vec4f& v) : v_(v) {}
inline Quaternion Quaternion::identity() {
	return Quaternion(0.f,0.f,0.f,1.f);
}
inline float Quaternion::norm() const {
	return v_.lengthSquared();
}
inline Quaternion Quaternion::operator* (const Quaternion& v) const {
	return v;//TODO
}
inline Quaternion Quaternion::operator* (float k) const { return Quaternion(v_*k); }
inline Quaternion Quaternion::operator+ (const Quaternion& v) const { return Quaternion(v_+v.v_); }
inline Quaternion Quaternion::operator- (const Quaternion& v) const { return Quaternion(v_-v.v_); }
inline vec4f Quaternion::v() const {
	return v_;
}
inline Quaternion Quaternion::rotateX(float theta) {
	return Quaternion(sinf(theta * 0.5f),0.f,0.f,cosf(theta * 0.5f));
}
inline Quaternion Quaternion::rotateY(float theta){
	return Quaternion(0.f,sinf(theta * 0.5f),0.f,cosf(theta * 0.5f));
}
inline Quaternion Quaternion::rotateZ(float theta){
	return Quaternion(0.f,0.f,sinf(theta * 0.5f),cosf(theta * 0.5f));
}
inline Quaternion Quaternion::rotate(vec3f axis,float theta) {
	float sin = sinf(theta * 0.5f);
	return Quaternion(sin*axis.x,sin*axis.y,sin*axis.z,cosf(theta * 0.5f));
}

#ifdef ARPHEG_ARCH_X86
	
	inline Quaternion::Quaternion(__m128 value) : v_(value) { }
	inline Quaternion Quaternion::conjugate() const {
		return Quaternion(-v_.x,-v_.y,-v_.z,v_.w);
	}

#else

inline Quaternion Quaternion::conjugate() const {
	return Quaternion(-v_.x,-v_.y,-v_.z,v_.w);
}

#endif

inline Quaternion Quaternion::lerp(const Quaternion& a,const Quaternion& b,float k) {
	return Quaternion( (a*(1.0f - k)) + (b*k) );
}
inline Quaternion Quaternion::slerp(const Quaternion& a,const Quaternion& b,float k) {
	auto av = a.v();
	auto angle = av.dot(b.v());
	if(angle < 0.0f) {
		angle = - angle;
		av =  av*-1.0f;
	}
	
	if(angle <= 0.99f){
		auto theta = acosf(angle);
		auto invsintheta = 1.0f / sinf(theta);
		auto ak = sinf(theta * (1.0f - k)) * invsintheta;
		auto bk = sinf(theta * k) * invsintheta;
		return Quaternion( av * ak + b.v() * bk );
	} else return lerp(a,b,k);
}
inline vec3f Quaternion::rotate(const vec3f& point) const {
	vec3f uv,uuv;
	vec3f qvec(v().x,v().y,v().z);
	uv = qvec.cross(point);
	uuv = qvec.cross(uv);
	uv = uv*(2.0f*v().w);
	uuv = uuv*2.0f;
	return point+uv+uuv;
}
