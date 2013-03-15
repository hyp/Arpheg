# Builds engine's unitests

macro (test name source)
	add_executable(${name}test ${source})
	target_link_libraries(${name}test ${ARPHEG_LIBS})
endmacro (test)

test(core "../src/core/unittest.cpp")
test(components "../src/components/unittest.cpp")
test(datatext  "../src/data/text/unittest.cpp")
test(ui "../src/ui/unittest.cpp")
