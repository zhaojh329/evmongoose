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
	int initialized;
	struct mg_serve_http_opts http_opts;
};

static int mg_ctx_destroy(lua_State *L)
{
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	
	if (ctx->initialized) {
		ctx->initialized = 0;
		mg_mgr_free(&ctx->mgr);
	}
    return 0;
}

static int mg_ctx_init(lua_State *L)
{
	struct mg_context *ctx = lua_newuserdata(L, sizeof(struct mg_context));
	
	ctx->L = L;
	ctx->initialized = 1;
	
    mg_mgr_init(&ctx->mgr, NULL);
    luaL_getmetatable(L, MONGOOSE_MT);
    lua_setmetatable(L, -2);

	if (lua_gettop(L) > 1) {
		struct ev_loop **loop = luaL_checkudata(L, 1, LOOP_MT);
		if (loop && *loop != UNINITIALIZED_DEFAULT_LOOP)
			mg_mgr_set_loop(&ctx->mgr, *loop);
	}
	
	return 1;
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct mg_mgr *mgr = nc->mgr;
	struct mg_context *ctx = container_of(mgr, struct mg_context, mgr);
	lua_State *L = ctx->L;

	lua_rawgeti(L, LUA_REGISTRYINDEX , ctx->callback);
	lua_pushinteger(L, (long)nc);

	lua_pushinteger(L, ev);

	lua_newtable(L);

	switch (ev) {
	case MG_EV_CLOSE:
	case MG_EV_CONNECT:
		lua_call(L, 3, 1);		
		break;
	
	case MG_EV_MQTT_CONNACK: {
		struct mg_mqtt_message *msg = (struct mg_mqtt_message *)ev_data;
		lua_pushinteger(L, msg->connack_ret_code);
		lua_setfield(L, -2, "connack_ret_code");	
		lua_call(L, 3, 1);
		break;
	}

	case MG_EV_MQTT_SUBACK:
	case MG_EV_MQTT_PUBACK: {
		struct mg_mqtt_message *msg = (struct mg_mqtt_message *)ev_data;
		lua_pushinteger(L, msg->message_id);
		lua_setfield(L, -2, "message_id");	
		lua_call(L, 3, 1);
		break;
	}

	case MG_EV_MQTT_PUBLISH: {
		struct mg_mqtt_message *msg = (struct mg_mqtt_message *)ev_data;
		lua_pushlstring(L, msg->topic.p, msg->topic.len);
		lua_setfield(L, -2, "topic");
		lua_pushlstring(L, msg->payload.p, msg->payload.len);
		lua_setfield(L, -2, "payload");
		lua_call(L, 3, 1);
		break;
	}

	case MG_EV_HTTP_REQUEST: {
		int i;
		char tmp[128];
		struct http_message *hm = (struct http_message *)ev_data;
		
		lua_pushlstring(L, hm->method.p, hm->method.len);
		lua_setfield(L, -2, "method");
		
		lua_pushlstring(L, hm->uri.p, hm->uri.len);
		lua_setfield(L, -2, "uri");
		
		lua_pushlstring(L, hm->proto.p, hm->proto.len);
		lua_setfield(L, -2, "proto");
		
		lua_pushlstring(L, hm->query_string.p, hm->query_string.len);
		lua_setfield(L, -2, "query_string");

		lua_newtable(L);

		for (i = 0; hm->header_names[i].len > 0; i++) {
			struct mg_str *h = &hm->header_names[i], *v = &hm->header_values[i];
			if (h->p) {
				lua_pushlstring(L,v->p, v->len);
				snprintf(tmp, sizeof(tmp), "%.*s", (int)h->len, h->p);
				lua_setfield(L, -2, tmp);
			}
		}
		
		lua_setfield(L, -2, "headers");
		
		lua_call(L, 3, 1);

		if (!lua_toboolean(L, -1))
			mg_serve_http(nc, hm, ctx->http_opts); /* Serve static content */

		break;
	}
	default:
		break;
	}

	lua_settop(L, 0);
}

static int lua_mg_bind(lua_State *L)
{
	int ref;
	struct mg_connection *nc;
	struct mg_bind_opts opts;
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	const char *address = luaL_checkstring(L, 2);
	const char *err;
	
	luaL_checktype(L, 3, LUA_TFUNCTION);
	
	memset(&opts, 0, sizeof(opts));

	opts.error_string = &err;
	
	if (lua_istable(L, 4)) {
		lua_getfield(L, 4, "document_root");
		ctx->http_opts.document_root = lua_tostring(L, -1);
	
		lua_getfield(L, 4, "ssl_cert");
		opts.ssl_cert = lua_tostring(L, -1);
	
		lua_getfield(L, 4, "ssl_key");
		opts.ssl_key = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_ca_cert");
		opts.ssl_ca_cert = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_cipher_suites");
		opts.ssl_cipher_suites = lua_tostring(L, -1);
	}

	lua_settop(L, 3);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	
	nc = mg_bind_opt(&ctx->mgr, address, ev_handler, opts);
	if (nc == NULL) {
		luaL_error(L, "%s", err);
		return 1;
	}
	
	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(nc);

	ctx->callback = ref;

	return 0;
}

static int lua_mg_connect(lua_State *L)
{
	int ref;
	struct mg_connection *nc;
	struct mg_connect_opts opts;
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	const char *address = luaL_checkstring(L, 2);
	const char *err;
	
	luaL_checktype(L, 3, LUA_TFUNCTION);
	
	memset(&opts, 0, sizeof(opts));

	opts.error_string = &err;
	
	if (lua_istable(L, 4)) {	
		lua_getfield(L, 4, "ssl_cert");
		opts.ssl_cert = lua_tostring(L, -1);
	
		lua_getfield(L, 4, "ssl_key");
		opts.ssl_key = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_ca_cert");
		opts.ssl_ca_cert = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_cipher_suites");
		opts.ssl_cipher_suites = lua_tostring(L, -1);
	}

	lua_settop(L, 3);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	
	nc = mg_connect_opt(&ctx->mgr, address, ev_handler, opts);
	if (nc == NULL) {
		luaL_error(L, "%s", err);
		return 1;
	}

	ctx->callback = ref;

	return 0;
}

static int lua_mg_set_protocol_mqtt(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	mg_set_protocol_mqtt(nc);
	return 0;
}

static int lua_mg_send_mqtt_handshake_opt(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	struct mg_send_mqtt_handshake_opts opts;
	char client_id[128] = "";
	
	memset(&opts, 0, sizeof(opts));

	sprintf(client_id, "evmongoose%ld", time(NULL));

	if (lua_istable(L, 3)) {
		lua_getfield(L, 3, "user_name");
		opts.user_name = lua_tostring(L, -1);

		lua_getfield(L, 3, "password");
		opts.password = lua_tostring(L, -1);

		lua_getfield(L, 3, "client_id");
		if (lua_tostring(L, -1))
			strncpy(client_id, lua_tostring(L, -1), sizeof(client_id));
	}

	mg_set_protocol_mqtt(nc);
	mg_send_mqtt_handshake_opt(nc, client_id, opts);
	return 0;
}

static int lua_mg_mqtt_subscribe(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	const char *topic = luaL_checkstring(L, 3);
	int msg_id = lua_tointeger(L, 4);
	struct mg_mqtt_topic_expression topic_expr = {NULL, 0};
	
	topic_expr.topic = topic;
	mg_mqtt_subscribe(nc, &topic_expr, 1, msg_id);
	return 0;
}
static int lua_mg_mqtt_publish(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	const char *topic = luaL_checkstring(L, 3);
	size_t len = 0;
	const char *payload = luaL_checklstring(L, 4, &len);
	int msgid = lua_tointeger(L, 5);
	int qos = lua_tointeger(L, 6);
	
	mg_mqtt_publish(nc, topic, msgid, MG_MQTT_QOS(qos), payload, len);
	return 0;
}

static int lua_mg_send_head(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	int status_code = luaL_checkint(L, 3);
	int64_t content_length = luaL_checkint(L, 4);
	const char *extra_headers = lua_tostring(L, 5);
	mg_send_head(nc, status_code, content_length, extra_headers);
	return 0;
}

static int lua_mg_print(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 3, &len);
	mg_send(nc, buf, len);
	return 0;
}

static int lua_mg_print_http_chunk(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 3, &len);
	mg_send_http_chunk(nc, buf, len);
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
	{"bind", lua_mg_bind},
	{"send_head", lua_mg_send_head},
	{"print", lua_mg_print},
	{"print_http_chunk", lua_mg_print_http_chunk},
	{"connect", lua_mg_connect},
	{"set_protocol_mqtt", lua_mg_set_protocol_mqtt},
	{"send_mqtt_handshake_opt", lua_mg_send_mqtt_handshake_opt},	
	{"mqtt_subscribe", lua_mg_mqtt_subscribe},
	{"mqtt_publish", lua_mg_mqtt_publish},
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

	lua_pushinteger(L, MG_EV_HTTP_REQUEST);
    lua_setfield(L, -2, "MG_EV_HTTP_REQUEST");

	lua_pushinteger(L, MG_EV_CONNECT);
    lua_setfield(L, -2, "MG_EV_CONNECT");

	lua_pushinteger(L, MG_EV_CLOSE);
    lua_setfield(L, -2, "MG_EV_CLOSE");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK");

	lua_pushinteger(L, MG_EV_MQTT_SUBACK);
    lua_setfield(L, -2, "MG_EV_MQTT_SUBACK");

	lua_pushinteger(L, MG_EV_MQTT_PUBACK);
    lua_setfield(L, -2, "MG_EV_MQTT_PUBACK");

	lua_pushinteger(L, MG_EV_MQTT_PUBLISH);
    lua_setfield(L, -2, "MG_EV_MQTT_PUBLISH");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK_ACCEPTED);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK_ACCEPTED");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK_UNACCEPTABLE_VERSION);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK_UNACCEPTABLE_VERSION");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK_IDENTIFIER_REJECTED);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK_IDENTIFIER_REJECTED");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK_BAD_AUTH);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK_BAD_AUTH");

	lua_pushinteger(L, MG_EV_MQTT_CONNACK_NOT_AUTHORIZED);
    lua_setfield(L, -2, "MG_EV_MQTT_CONNACK_NOT_AUTHORIZED");
	
    return 1;
}

