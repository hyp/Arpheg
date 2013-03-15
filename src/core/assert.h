#pragma once

#include <assert.h>

//TODO

#ifdef _MSC_VER

	#ifdef  __cplusplus
	extern "C" {
	#endif

	_CRTIMP void __cdecl _wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);

	#ifdef  __cplusplus
	}
	#endif

	#define assertRelease(expression)  (void)( (!!(expression)) || (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), __LINE__), 0) )
#else
	#define assertRelease(expression) ((void)0)
#endif
