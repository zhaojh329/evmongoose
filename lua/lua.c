#include "mongoose.h"
#include <lauxlib.h>
#include <lualib.h>

#define MONGOOSE_NAME "mongoose"

struct mg_context {
    struct mg_mgr mgr;
    lua_State *vm;
};

static int mg_ctx_destroy(lua_State *L)
{
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_NAME);
	mg_mgr_free(&ctx->mgr);
    return 0;
}

static int mg_ctx_init(lua_State *L)
{
	struct mg_context *ctx = lua_newuserdata(L, sizeof(struct mg_context));

    mg_mgr_init(&ctx->mgr, NULL);

    luaL_getmetatable(L, MONGOOSE_NAME);
    lua_setmetatable(L, -2);

    ctx->vm = L;
	
	return 1;
}

static int mg_ctx_bind(lua_State *L)
{
	return 1;
}

#if LUA_VERSION_NUM==501
/*
** Adapted from Lua 5.2
*/
void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup+1, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    lua_pushstring(L, l->name);
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -(nup+1));
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_settable(L, -(nup + 3));
  }
  lua_pop(L, nup);  /* remove upvalues */
}
#endif

int luaopen_evmongoose(lua_State *L) 
{
	static const luaL_Reg mongoose_meta[] =
	{
		{"__gc", mg_ctx_destroy},
		{"bind", mg_ctx_bind},
		{NULL, NULL}
	};

	/* metatable.__index = metatable */
    luaL_newmetatable(L, MONGOOSE_NAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, mongoose_meta, 0);

    lua_createtable(L, 0, 1);

    lua_pushcfunction(L, mg_ctx_init);
    lua_setfield(L, -2, "init");
	
    return 1;
}

