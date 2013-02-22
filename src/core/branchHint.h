#pragma once

#ifdef __GNUC__
	#define LIKELY_TRUE(expr) __builtin_expect((expr),1)
	#define LIKELY_FALSE(expr) __builtin_expect((expr),0)
#else
	#define LIKELY_TRUE(expr) (expr)
	#define LIKELY_FALSE(expr) (expr)
#endif