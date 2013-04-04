
if(NOT LUA_FOUND)
	set(LUA_LIB_SRC_FILES
		lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c
		ltable.c ltm.c lundump.c lvm.c lzio.c lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c lmathlib.c loslib.c lstrlib.c ltablib.c loadlib.c linit.c)
	foreach(i ${LUA_LIB_SRC_FILES})
		list(APPEND LUA_SRC_FILES "../src/dependencies/lua/src/${i}")
	endforeach(i)
	add_library(lualib ${LUA_SRC_FILES})

	set(LUA_FOUND 1)
	set(LUA_LIBRARY lualib)
	set(LUA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../src/dependencies/lua/src")
endif()