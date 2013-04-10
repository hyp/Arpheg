//rotate vector
vec3 quaternionRotate(vec4 q, vec3 v)       {
        return v + 2.0*cross(q.xyz, cross(q.xyz,v) + q.w*v);
}
//combine quaternions
vec4 quaternionMul(vec4 a, vec4 b)       {
        return vec4(cross(a.xyz,b.xyz) + a.xyz*b.w + b.xyz*a.w, a.w*b.w - dot(a.xyz,b.xyz));
}
//inverse quaternion
vec4 quatenionInverse(vec4 q)       {
        return vec4(-q.xyz,q.w);
}
