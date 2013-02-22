# Provides a list of files for the core library 

set(ARPHEG_CORE_FILES
	math.cpp 
	memory.cpp
	timing.cpp
	typeDescriptor.cpp
	io.cpp
	murmurHash.cpp
	text.cpp
	charSource.cpp
	utf.cpp
	bufferStringStream.cpp
	thread.cpp
	logging.cpp
	random/mersenneTwister.cpp
)
foreach(i ${ARPHEG_CORE_FILES})
	list(APPEND ARPHEG_FILES "../src/core/${i}")
endforeach(i)
