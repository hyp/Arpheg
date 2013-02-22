# Compiles the engine into a static library

add_library(arpheglib ${ARPHEG_FILES})
list(APPEND ARPHEG_LIBS arpheglib)
