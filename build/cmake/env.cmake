# Configures the build environment

if (MSVC)
	add_definitions(/arch:SSE2 /W4)
else()
	add_definitions(-std=c++0x -msse2 -Wall)
endif()
