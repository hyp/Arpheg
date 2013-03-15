# Configures the build environment

if (MSVC)
	add_definitions(/arch:SSE2)
else()
	add_definitions(-std=c++0x -msse2)
endif()
