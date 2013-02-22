# Configures the build environment

if (MSVC)
else()
	add_definitions(-std=c++0x -msse2)
endif()
