# Link assimp

if(NOT ASSIMP_FOUND)
	set(SKIP_INSTALL_ALL ON)

	set(BUILD_STATIC_LIB ON CACHE BOOL "Static Library")
	set(BUILD_STATIC_LIB ON)
	set(ENABLE_BOOST_WORKAROUND ON CACHE BOOL "Assimp no boost" )
	set(ENABLE_BOOST_WORKAROUND ON)
	set(BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Assimp no tools")
	set(BUILD_ASSIMP_TOOLS OFF)
	set(BUILD_ASSIMP_SAMPLES OFF CACHE BOOL "Assimp no samples")
	set(BUILD_ASSIMP_SAMPLES OFF)


	#This is required to link with ASSIMP in debug mode
	if ( MSVC)
		add_definitions( -D_SCL_SECURE_NO_WARNINGS -D_HAS_ITERATOR_DEBUGGING=0)
	endif()

	add_subdirectory("${CMAKE_SOURCE_DIR}/../src/dependencies/assimp" assimp)
	
	set(ASSIMP_FOUND 1)
	set(ASSIMP_LIBRARY assimp)
	#set(LIBPNG_INCLUDE_DIR ../libpng/ "${CMAKE_BINARY_DIR}/zlib")
	
endif()
