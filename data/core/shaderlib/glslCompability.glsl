//GLSL compability between GL(es) 2 and GL(es) 3+

#ifdef COMPABILITY_NO_PRECISION
	#define lowp
	#define mediump
	#define highp
#endif

#ifdef COMPABILITY_MODERN_GLSL
	#define ATTRIBUTE_IN in
	#define VARYING_IN in
	#define VARYING_OUT out
	#define DEF_PIXEL_SHADER_OUT out vec4 arpheg_ps_out
	#define PIXEL_SHADER_OUT arpheg_ps_out
#else
	#define ATTRIBUTE_IN attribute
	#define VARYING_IN varying
	#define VARYING_OUT varying
	#define DEF_PIXEL_SHADER_OUT
	#define PIXEL_SHADER_OUT gl_FragColor
#endif