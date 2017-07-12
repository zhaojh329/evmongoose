find_package(Lua51)

set(LUA_SUPPORT_DEFAULT "ON")

if (NOT LUA51_FOUND)
	set(LUA_SUPPORT_DEFAULT "OFF")
endif (NOT LUA51_FOUND)

option(LUA_SUPPORT "whether to support Lua" ${LUA_SUPPORT_DEFAULT})

if(NOT LUAPATH)
	execute_process(
		COMMAND  lua -e "for k in string.gmatch(package.cpath .. \";\", \"([^;]+)/..so;\") do if k:sub(1,1) == \"/\" then print(k) break end end"

		OUTPUT_VARIABLE LUAPATH
		RESULT_VARIABLE LUA_CHECK_RES
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(LUA_SUPPORT)
		if(NOT ${LUA_CHECK_RES} EQUAL 0 OR "${LUAPATH}" EQUAL "")
			message(FATAL_ERROR "Lua was not found on your system")
		endif()
	endif()
endif()

if(LUA_SUPPORT)
	include_directories(${LUA_INCLUDE_DIR})
	
	add_library(evmongoose-lua MODULE lua_mongoose.c lua_posix.c)
	set_target_properties(evmongoose-lua PROPERTIES OUTPUT_NAME evmongoose PREFIX "")
		
	target_link_libraries(evmongoose-lua evmongoose util)

	install(TARGETS evmongoose-lua
		LIBRARY DESTINATION ${LUAPATH}
	)
endif(LUA_SUPPORT)
