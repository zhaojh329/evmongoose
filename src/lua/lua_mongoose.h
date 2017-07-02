#ifndef __LUA_MONGOOSE_H
#define __LUA_MONGOOSE_H

#include <lauxlib.h>
#include <lualib.h>

#ifndef container_of
#define container_of(ptr, type, member)					\
	({								\
		const typeof(((type *) NULL)->member) *__mptr = (ptr);	\
		(type *) ((char *) __mptr - offsetof(type, member));	\
	})
#endif

#define EVMG_LUA_ADD_VARIABLE(v)	{ \
		lua_pushinteger(L, v); \
		lua_setfield(L, -2, #v); \
	}

int luaopen_evmongoose_syslog(lua_State *L);

#endif
