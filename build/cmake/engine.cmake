# Add dependencies

if (MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set(LINK_FLAGS "/MACHINE:X64")
		set(STATIC_LIBRARY_FLAGS "/MACHINE:X64")
	endif()
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/dependencies/" ${CMAKE_MODULE_PATH})

find_package(ZLIB REQUIRED)
list(APPEND ARPHEG_LIBS ${ZLIB_LIBRARY})
include_directories(${ZLIB_INCLUDE_DIR})
find_package(ASSIMP REQUIRED)
list(APPEND ARPHEG_LIBS ${ASSIMP_LIBRARY})

# Provides a list of files for the engine

set(ARPHEG_ENGINE_FILES
	application/application.cpp
	application/services.cpp
	application/mainWindow.cpp	
	application/timing.cpp
	application/logging.cpp
	application/tasking.cpp
	application/profiling.cpp
	
	input/input.cpp
	
	rendering/opengl/context.cpp
	rendering/opengl/rendering.cpp
	rendering/opengl/textures.cpp
	rendering/opengl/api.cpp
	rendering/types.cpp
	rendering/debug.cpp
	rendering/ui.cpp
	rendering/2d.cpp
	rendering/geometryBatcher.cpp
	rendering/text.cpp
	rendering/softwareOcclusion/softwareOcclusion.cpp
	rendering/animation.cpp

	data/data.cpp
	data/image/reader.cpp
	data/text/parser.cpp
	data/shader/preprocess.cpp
	data/font/font.cpp
	data/intermediate/mesh/reader.cpp
	data/intermediate/bundle/parser.cpp
	data/intermediate/font/reader.cpp
	data/utils/path.cpp
	
	ui/ui.cpp
	ui/widget.cpp
	ui/events.cpp
	ui/components.cpp
	
	collisions/collisions.cpp
	
	components/state.cpp
	
	dependencies/stb/stb_image.c
)
foreach(i ${ARPHEG_ENGINE_FILES})
	list(APPEND ARPHEG_FILES "../src/${i}")
endforeach(i)

if (WIN32)
	list(APPEND ARPHEG_LIBS opengl32)
else ()
	list(APPEND ARPHEG_LIBS GL X11 rt) #NB: RT is for the clock
endif()
