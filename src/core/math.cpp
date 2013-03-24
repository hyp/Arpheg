#include "math.h"


inline void mat44f::multiply(const mat44f& m1,const mat44f& m2,mat44f& result){
#ifdef ARPHEG_ARCH_X86
	auto m1a = m1.a.m128();
	auto m1b = m1.b.m128();
	auto m1c = m1.c.m128();
	auto m1d = m1.d.m128();

	auto v2 = m2.a.m128();
	auto r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	result.a =  _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));

	v2 = m2.b.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	result.b = _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));

	v2 = m2.c.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	result.c = _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));

	v2 = m2.d.m128();
	r = _mm_mul_ps(m1a,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(0,0,0,0)));
	r = _mm_add_ps(r,_mm_mul_ps(m1b,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(1,1,1,1))));
	r = _mm_add_ps(r,_mm_mul_ps(m1c,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,2,2,2))));
	result.d = _mm_add_ps(r,_mm_mul_ps(m1d,_mm_shuffle_ps(v2,v2,_MM_SHUFFLE(3,3,3,3))));
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
inline void mat44f::multiply(mat44f& m1,const mat44f& m2){
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

mat44f mat44f::identity() {
	return mat44f(vec4f(1.0f,0.0f,0.0f,0.0f), vec4f(0.0f,1.0f,0.0f,0.0f), vec4f(0.0f,0.0f,1.0f,0.0f), vec4f(0.0f,0.0f,0.0f,1.0f) );
}
mat44f mat44f::translate(vec3f x) {
#ifdef PLATFORM_MATH_MAT_ROWMAJOR
	return mat44f(
		vec4f(1.0f,0.0f,0.0,x.x),
		vec4f(0.0f,1.0f,0.0,x.y),
		vec4f(0.0,0.0,1.0,x.z),
		vec4f(0.0f,0.0f,0.0f,1.0f) );
#else
	return mat44f(
		vec4f(1.0f,0.0f,0.0,0.0),
		vec4f(0.0f,1.0f,0.0,0.0),
		vec4f(0.0,0.0,1.0,0.0),
		vec4f(x.x,x.y,x.z,1.0f) );
#endif
}
mat44f mat44f::scale(vec3f x) {
	return mat44f(
		vec4f(x.x,0.0f,0.0,0.0),
		vec4f(0.0f,x.y,0.0,0.0),
		vec4f(0.0,0.0,x.z,0.0),
		vec4f(0.0,0.0,0.0,1.0f) );
}
mat44f mat44f::rotate(const Quaternion& rotation) {
	//Quaternion to matrix.
#ifdef ARPHEG_ARCH_X86
	auto v = rotation.v().m128();
	auto temp = _mm_mul_ps(v,_mm_shuffle_ps(v,v,_MM_SHUFFLE(2,2,1,0)));
	auto xxyyzzzw2 = vec4f( _mm_add_ps(temp,temp) ); 
	temp = _mm_mul_ps(v,_mm_shuffle_ps(v,v,_MM_SHUFFLE(1,1,0,2)));
	auto xzxyyzyw2 = vec4f( _mm_add_ps(temp,temp) ); 
	auto xw2 = vec4f( _mm_mul_ss(v, _mm_shuffle_ps(v,v,_MM_SHUFFLE(3,3,3,3))) ).x * 2.0f;
#else
	auto v = rotation.v();
	auto temp = v * vec4f(v.x,v.y,v.z,v.z);
	auto xxyyzzzw2 = temp + temp;
	temp = v * vec4f(v.z,v.x,v.y,v.y);
	auto xzxyyzyw2 = temp + temp;	
	auto xw2 = v.x * v.w * 2.0f;
#endif

#ifdef PLATFORM_MATH_MAT_ROWMAJOR
	return mat44f(
		vec4f(1.0f - (xxyyzzzw2.y + xxyyzzzw2.z),(xzxyyzyw2.y + xxyyzzzw2.w),(xzxyyzyw2.x - xzxyyzyw2.w),0.0f),
		vec4f(xzxyyzyw2.y - xxyyzzzw2.w,1.0f - (xxyyzzzw2.x + xxyyzzzw2.z),(xzxyyzyw2.z + xw2),0.0f),
		vec4f(xzxyyzyw2.x + xzxyyzyw2.w,xzxyyzyw2.z - xw2,1.0f - (xxyyzzzw2.x + xxyyzzzw2.y)  ,0.0f),
		vec4f(0.0f,0.0f,0.0f,1.0f) );//TODO transpose.
#else
	return mat44f(
		vec4f(1.0f - (xxyyzzzw2.y + xxyyzzzw2.z),(xzxyyzyw2.y + xxyyzzzw2.w),(xzxyyzyw2.x - xzxyyzyw2.w),0.0f),
		vec4f(xzxyyzyw2.y - xxyyzzzw2.w,1.0f - (xxyyzzzw2.x + xxyyzzzw2.z),(xzxyyzyw2.z + xw2),0.0f),
		vec4f(xzxyyzyw2.x + xzxyyzyw2.w,xzxyyzyw2.z - xw2,1.0f - (xxyyzzzw2.x + xxyyzzzw2.y)  ,0.0f),
		vec4f(0.0f,0.0f,0.0f,1.0f) );
#endif
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
	mat44f result = scale(scaling);
	multiply(result,translate(translation));
	multiply(result,rotate(rotation));
	return result;
}

//Special view + projection matrices.
mat44f mat44f::ortho(vec2f min,vec2f max,float near,float far) {
	const float right = max.x, left = min.x, top = min.y, bottom = max.y;
	float tx = - (right + left) / (right - left);
	float ty = - (top + bottom) / (top - bottom);
	float tz = - (far + near)   / (far - near);

#ifdef PLATFORM_MATH_MAT_ROWMAJOR
	return mat44f(
		vec4f(2.0f/(right - left),0.0,0.0,tx),
		vec4f(0.0,2.0f/(top - bottom),0.0,ty),
		vec4f(0.0,0.0,(-2.0f)/(far-near),tz),
		vec4f(0.0,0.0,0.0,1.0f) );
#else
	return mat44f(
		vec4f(2.0f/(right - left),0.0,0.0,0.0),
		vec4f(0.0,2.0f/(top - bottom),0.0,0.0),
		vec4f(0.0,0.0,(-2.0f)/(far-near),0.0),
		vec4f(tx,ty,tz,1.0f) );
#endif
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
    /*return mat44f (
			vec4f(f / aspect, 0, 0, 0),
            vec4f(0, f, 0, 0),
            vec4f(0, 0, A, -1),
            vec4f(0, 0, B, 1));*/
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
