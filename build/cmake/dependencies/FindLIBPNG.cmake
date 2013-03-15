# Link libpng

if(NOT LIBPNG_FOUND)
	set(PNG_STATIC ON)
	set(PNG_SHARED OFF)
	set(PNG_TESTS OFF)
	set(SKIP_INSTALL_ALL ON)

	add_subdirectory("${CMAKE_SOURCE_DIR}/../src/dependencies/libpng" libpng)

	set(LIBPNG_FOUND 1)
	set(LIBPNG_LIBRARY png15_static)
	set(LIBPNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../src/dependencies/libpng" "${CMAKE_BINARY_DIR}/libpng")
endif()
