# Provides a list of files for the core library 

set(ARPHEG_CORE_FILES
	math.cpp 
	memory.cpp
	time.cpp
	typeDescriptor.cpp
	io.cpp
	text.cpp
	charSource.cpp
	utf.cpp
	bufferStringStream.cpp
	thread/thread.cpp
	
	random/mersenneTwister.cpp
	hash/murmur.cpp
)
foreach(i ${ARPHEG_CORE_FILES})
	list(APPEND ARPHEG_FILES "../src/core/${i}")
endforeach(i)
