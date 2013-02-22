# Provides a list of files for the engine

set(ARPHEG_ENGINE_FILES
	application/application.cpp
	application/services.cpp
	application/mainWindow.cpp	
	
	input/input.cpp
	
	rendering/opengl/context.cpp
	rendering/opengl/rendering.cpp
	rendering/opengl/textures.cpp
	rendering/opengl/api.cpp
	rendering/types.cpp
	rendering/geometryBatcher.cpp
	rendering/text.cpp

	resources/resources.cpp
	resources/image/reader.cpp
	resources/mesh/reader.cpp
	resources/font.cpp
	
	collisions/collisions.cpp
	
	contouring/net.cpp
	contouring/octreeNet.cpp
)
foreach(i ${ARPHEG_ENGINE_FILES})
	list(APPEND ARPHEG_FILES "../src/${i}")
endforeach(i)

if (WIN32)
	list(APPEND ARPHEG_LIBS opengl32)
else ()
	list(APPEND ARPHEG_LIBS GL X11)
endif()
