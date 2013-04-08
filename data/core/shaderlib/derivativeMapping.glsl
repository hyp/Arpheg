//GLSL derivative bumpmapping implementation 
//See http://mmikkelsen3d.blogspot.com

// Project the surface gradient (dhdx, dhdy) onto the surface (n, dpdx, dpdy)
vec3 CalculateSurfaceGradient(vec3 n, vec3 dpdx, vec3 dpdy, float dhdx, float dhdy) {
    vec3 r1 = cross(dpdy, n);
    vec3 r2 = cross(n, dpdx);
 
    return (r1 * dhdx + r2 * dhdy) / dot(dpdx, r1);
}
// Move the normal away from the surface normal in the opposite surface gradient direction
vec3 PerturbNormal(vec3 n, vec3 dpdx, vec3 dpdy, float dhdx, float dhdy) {
    return normalize(n - CalculateSurfaceGradient(n, dpdx, dpdy, dhdx, dhdy));
}

// Calculate the surface normal using screen-space partial derivatives of the height field
vec3 CalculateSurfaceNormal(vec3 position, vec3 normal, float height) {
    vec3 dpdx = dFdx(position);
    vec3 dpdy = dFdy(position);
 
    float dhdx = dFdx(height);
    float dhdy = dFdy(height);
 
    return PerturbNormal(normal, dpdx, dpdy, dhdx, dhdy);
}