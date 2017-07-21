/*
 * Copyright (C) 2017 jianhui zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LUA_MONGOOSE_H
#define __LUA_MONGOOSE_H

#include <lauxlib.h>
#include <lualib.h>

/* Compatibility defines */		
#if LUA_VERSION_NUM <= 501
		
#define lua_setuservalue(L, i) lua_setfenv((L), (i))		
#define lua_getuservalue(L, i) lua_getfenv((L), (i))

/* NOTE: this only works if nups == 0! */
#define luaL_setfuncs(L, fns, nups) luaL_register((L), NULL, (fns))

#define lua_rawlen(L, i) lua_objlen((L), (i))

#endif

#define EVMG_LUA_SETCONST(v)	{ \
		lua_pushinteger(L, v); \
		lua_setfield(L, -2, #v); \
	}

int luaopen_evmongoose_posix(lua_State *L);

#endif
