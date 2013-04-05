# Compiles the data import dynamic library.

find_package(ASSIMP REQUIRED)
set(ARPHEG_DATALIB_FILES
	application/logging.cpp
	rendering/types.cpp
	data/3d.cpp
	data/utils/path.cpp
	data/intermediate/datalib.cpp
	data/intermediate/mesh/reader.cpp
)
foreach(i ${ARPHEG_DATALIB_FILES})
	list(APPEND ARPHEG_FILES_DATALIB "../src/${i}")
endforeach(i)
add_library(arphegdataimport SHARED ${ARPHEG_FILES_CORE} ${ARPHEG_FILES_DATALIB})
target_link_libraries(arphegdataimport ${ZLIB_LIBRARY} ${ASSIMP_LIBRARY})
#if (NOT WIN32)
#message("MESSAGE = ${CMAKE_SOURCE_DIR}/../bin")
#install(TARGETS arphegdataimport DESTINATION "${CMAKE_SOURCE_DIR}/../bin")
#endif()
