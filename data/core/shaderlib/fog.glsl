//GLSL fog implementation

vec3 fog(vec3 color, vec3 fogcolor, float depth, float density){
    const float e = 2.71828182845904523536028747135266249;
    float f = pow(e, -pow(depth*density, 2));
    return mix(fogcolor, color, f);
}
