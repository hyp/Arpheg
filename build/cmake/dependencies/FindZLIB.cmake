# Link zlib

if(NOT ZLIB_FOUND)
	set(SKIP_INSTALL_ALL ON)

	add_subdirectory("${CMAKE_SOURCE_DIR}/../src/dependencies/zlib" zlib)

	# MSVC10 Bug fix
	if (MSVC)
		if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
			set_target_properties(zlibstatic PROPERTIES LINK_FLAGS "/MACHINE:X64")
			set_target_properties(zlibstatic PROPERTIES STATIC_LIBRARY_FLAGS "/MACHINE:X64")
		endif()
	endif()

	set(ZLIB_FOUND 1)
	set(ZLIB_LIBRARY zlibstatic)
	set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../src/dependencies/zlib" "${CMAKE_BINARY_DIR}/zlib")
endif()
