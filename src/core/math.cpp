#include "math.h"

inline static void matrixMultiply(const mat44f& m1,const mat44f& m2,mat44f& result){
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
}
mat44f mat44f::operator * (const mat44f& other) const {
	mat44f result;
	matrixMultiply(*this,other,result);
	return result;
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
#define MatrixIndex(i,j) i*4 + j //transposed
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
	matrixMultiply(projectionView,model,result);
	return result;
}
mat44f mat44f::rotate(const Quaternion& rotation) {
	//TODO: Can be done with 2 sse muls..
	vec4f vv = rotation.v() * rotation.v();

	float xy = rotation.v().x*rotation.v().y;
	float xz = rotation.v().x*rotation.v().z;
	float xw = rotation.v().x*rotation.v().w;

	float yz = rotation.v().y*rotation.v().z;
	float yw = rotation.v().y*rotation.v().w;
	float zw = rotation.v().z*rotation.v().w;

#ifdef PLATFORM_MATH_MAT_ROWMAJOR
	return mat44f(
		vec4f(1.0f - 2.0f*(vv.y + vv.z),2.0f*(xy - zw),2.0f*(xz + yw),0.0f),
		vec4f(2.0f*(xy + zw),1.0f - 2.0f*(vv.x + vv.z),2.0f*(yz - xw),0.0f),
		vec4f(2.0f*(xz - yw),2.0f*(yz+xw),1.0f - 2.0f*(vv.x + vv.y)  ,0.0f),
		vec4f(0.0f,0.0f,0.0f,1.0f) );
#else
	return mat44f(
		vec4f(1.0f - 2.0f*(vv.y + vv.z),2.0f*(xy + zw),2.0f*(xz - yw),0.0f),
		vec4f(2.0f*(xy - zw),1.0f - 2.0f*(vv.x + vv.z),2.0f*(yz+xw),0.0f),
		vec4f(2.0f*(xz + yw),2.0f*(yz - xw),1.0f - 2.0f*(vv.x + vv.y)  ,0.0f),
		vec4f(0.0f,0.0f,0.0f,1.0f) );
#endif
}
