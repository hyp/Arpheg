# Builds engine's unitests

macro (test name source)
	add_executable(${name}test ${source})
	target_link_libraries(${name}test ${ARPHEG_LIBS})
endmacro (test)

test(core "../src/core/unittest.cpp")
test(contouring "../src/contouring/unittest.cpp")
