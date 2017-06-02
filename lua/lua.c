#include "mongoose.h"
#include <lauxlib.h>
#include <lualib.h>

#ifndef container_of
#define container_of(ptr, type, member)					\
	({								\
		const typeof(((type *) NULL)->member) *__mptr = (ptr);	\
		(type *) ((char *) __mptr - offsetof(type, member));	\
	})
#endif
	
#define LOOP_MT    "ev{loop}"
#define UNINITIALIZED_DEFAULT_LOOP (struct ev_loop*)1
#define MONGOOSE_MT "mongoose"

struct mg_context {
    struct mg_mgr mgr;
    lua_State *L;
	int callback;
};

static int mg_ctx_destroy(lua_State *L)
{
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	printf("mg_ctx_destroy:%p\n", ctx);
	mg_mgr_free(&ctx->mgr);
    return 0;
}

static int mg_ctx_init(lua_State *L)
{
	struct ev_loop **loop = NULL;
	struct mg_context *ctx;
	
	if (lua_gettop(L) > 0) {
		loop = luaL_checkudata(L, 1, LOOP_MT);
	}
	
	ctx = lua_newuserdata(L, sizeof(struct mg_context));
    mg_mgr_init(&ctx->mgr, NULL);
    luaL_getmetatable(L, MONGOOSE_MT);
    lua_setmetatable(L, -2);

	if (loop && *loop != UNINITIALIZED_DEFAULT_LOOP) {
		mg_mgr_set_loop(&ctx->mgr, *loop);
		printf("set loop:%p\n", *loop);
	}
	
    ctx->L = L;
	
	printf("mg_ctx_init:%p\n", ctx);
	
	return 1;
}

static struct mg_serve_http_opts s_http_server_opts;

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct mg_mgr *mgr = nc->mgr;
	struct mg_context *ctx = container_of(mgr, struct mg_context, mgr);
	lua_State *L = ctx->L;	
	
	if (ev == MG_EV_HTTP_REQUEST) {
		lua_rawgeti(L, LUA_REGISTRYINDEX , ctx->callback);
		lua_pushinteger(L, (long)nc);
		lua_pcall(L, 1, 1, 0);
	}
}

static int mg_ctx_bind(lua_State *L)
{
	int ref;
	struct mg_connection *nc;
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	const char *address = luaL_checkstring(L, 2);
	
	luaL_checktype(L, 3, LUA_TFUNCTION);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	
	nc = mg_bind(&ctx->mgr, address, ev_handler);
	if (nc == NULL) {
		luaL_error(L, "Failed to bind:%s", address);
		return 1;
	}
	
	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.document_root = ".";  // Serve current directory
	s_http_server_opts.enable_directory_listing = "yes";

	ctx->callback = ref;
	
	return 1;
}

static int mg_ctx_printf(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	const char *buf = luaL_checkstring(L, 3);
	mg_printf(nc, "%s", buf);
	return 0;
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

static const luaL_Reg mongoose_meta[] = {
	{"__gc", mg_ctx_destroy},
	{"destroy", mg_ctx_destroy},
	{"bind", mg_ctx_bind},
	{"printf", mg_ctx_printf},
	{NULL, NULL}
};
	
int luaopen_evmongoose(lua_State *L) 
{
	/* metatable.__index = metatable */
    luaL_newmetatable(L, MONGOOSE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, mongoose_meta, 0);

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, mg_ctx_init);
    lua_setfield(L, -2, "init");
	
    return 1;
}

