// 

vec3 visualizeHeat(float intensity){
	//Gradient
	vec3 colors[3];
	colors[0] = vec3(0.,0.,1.);
	colors[1] = vec3(1.,1.,0.);
	colors[2] = vec3(1.,0.,0.);
	int ix = (intensity < 0.5)? 0:1;
	return mix(colors[ix],colors[ix+1],(intensity-float(ix)*0.5)*2.0);
}
