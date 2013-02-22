# Builds engine's samples

macro (sample name)
	add_executable(${name}sample "../samples/${name}/main.cpp")
	target_link_libraries(${name}sample ${ARPHEG_LIBS})
endmacro(sample)

sample(gl)
sample(triangle)
sample(resources)
