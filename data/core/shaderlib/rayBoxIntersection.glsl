// GLSL ray box intersection test.

bool IntersectBox(vec3 ro,vec3 rd, vec3 boxMin,vec3 boxMax, out float t0, out float t1) {
	vec3 invR = 1.0 / rd;
	vec3 tbot = invR * (boxMin-ro);
	vec3 ttop = invR * (boxMax-ro);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = max(ttop, tbot);
	vec2 t = max(tmin.xx, tmin.yz);
	t0 = max(t.x, t.y);
	t = min(tmax.xx, tmax.yz);
	t1 = min(t.x, t.y);
	return t0 <= t1;
}
