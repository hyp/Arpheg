#pragma once

#ifdef _MSC_VER
#define THREAD_LOCAL(T,n) __declspec(thread) T n
#else
#define THREAD_LOCAL(T,n) __thread T n
#endif
