#include "mongoose.h"
#include "list.h"
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

struct mg_bind_ctx {
	struct mg_serve_http_opts http_opts;
	struct mg_connection *nc;
	struct list_head node;
};

struct mg_context {
    struct mg_mgr mgr;
    lua_State *L;
	int callback;
	struct list_head bind_ctx_list;
	int initialized;
};

static struct mg_bind_ctx *find_bind_ctx(struct mg_context *ctx, struct mg_connection *nc)
{
	struct mg_bind_ctx *bind = NULL;
	
	list_for_each_entry(bind, &ctx->bind_ctx_list, node) {
		if (bind->nc == nc)
			return bind;
	}
	return NULL;
}

static int mg_ctx_destroy(lua_State *L)
{
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	
	if (ctx->initialized) {
		struct mg_bind_ctx *bind, *tmp;
	
		list_for_each_entry_safe(bind, tmp, &ctx->bind_ctx_list, node) {
			list_del(&bind->node);
			free(bind);
		}
	
		mg_mgr_free(&ctx->mgr);

		ctx->initialized = 0;
	}
    return 0;
}

static int mg_ctx_init(lua_State *L)
{
	struct mg_context *ctx = lua_newuserdata(L, sizeof(struct mg_context));
	
	ctx->L = L;
	ctx->initialized = 1;

	INIT_LIST_HEAD(&ctx->bind_ctx_list);
		
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

static void ev_http_reply(struct mg_context *ctx, struct mg_connection *nc, void *ev_data)
{
	lua_State *L = ctx->L;
	struct http_message *rsp = (struct http_message *)ev_data;
	int i;
	char tmp[128];

	nc->flags |= MG_F_CLOSE_IMMEDIATELY;

	lua_pushinteger(L, rsp->resp_code);
	lua_setfield(L, -2, "resp_code");

	lua_pushlstring(L, rsp->resp_status_msg.p, rsp->resp_status_msg.len);
	lua_setfield(L, -2, "resp_status_msg");

	lua_newtable(L);

	for (i = 0; rsp->header_names[i].len > 0; i++) {
		struct mg_str *h = &rsp->header_names[i], *v = &rsp->header_values[i];
		if (h->p) {
			lua_pushlstring(L,v->p, v->len);
			snprintf(tmp, sizeof(tmp), "%.*s", (int)h->len, h->p);
			lua_setfield(L, -2, tmp);
		}
	}
	
	lua_setfield(L, -2, "headers");

	lua_pushlstring(L, rsp->body.p, rsp->body.len);
	lua_setfield(L, -2, "body");
	
	lua_call(L, 3, 1);
}

static void ev_http_request(struct mg_context *ctx, struct mg_connection *nc, void *ev_data)
{
	lua_State *L = ctx->L;
	struct mg_bind_ctx *bind = find_bind_ctx(ctx, nc->listener);
	struct http_message *hm = (struct http_message *)ev_data;
	int i;
	char tmp[128];

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
		mg_serve_http(nc, hm, bind->http_opts); /* Serve static content */
}

static void ev_websocket_frame(struct mg_context *ctx, struct mg_connection *nc, void *ev_data)
{
	lua_State *L = ctx->L;
	struct websocket_message *wm = (struct websocket_message *)ev_data;

	lua_pushlstring(L, (const char *)wm->data, wm->size);
	lua_setfield(L, -2, "data");

	if (wm->flags & WEBSOCKET_OP_TEXT) {
		lua_pushinteger(L, WEBSOCKET_OP_TEXT);
		lua_setfield(L, -2, "op");
	} else if (wm->flags & WEBSOCKET_OP_BINARY) {
		lua_pushinteger(L, WEBSOCKET_OP_BINARY);
		lua_setfield(L, -2, "op");
	}

	lua_call(L, 3, 1);
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
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
		lua_call(L, 3, 1);
		break;

	case MG_EV_RECV: {		
		struct mbuf *io = &nc->recv_mbuf;

		lua_pushlstring(L, io->buf, io->len);
		lua_setfield(L, -2, "data");
		mbuf_remove(io, io->len);
		lua_call(L, 3, 1);

		if (lua_toboolean(L, -1))
			mbuf_remove(io, io->len);
		break;
	}
		
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

	case MG_EV_HTTP_REQUEST:
		ev_http_request(ctx, nc, ev_data);
		break;

	case MG_EV_HTTP_REPLY:
		ev_http_reply(ctx, nc, ev_data);		
		break;

	case MG_EV_WEBSOCKET_FRAME:
		ev_websocket_frame(ctx, nc, ev_data);
		break;
	
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
	struct mg_bind_ctx *bind = NULL;
	const char *proto = NULL;
	const char *err;
	
	luaL_checktype(L, 3, LUA_TFUNCTION);
	
	memset(&opts, 0, sizeof(opts));
	opts.error_string = &err;

	bind = calloc(1, sizeof(struct mg_bind_ctx));
	if (!bind)
		luaL_error(L, "%s", strerror(errno));
	
	if (lua_istable(L, 4)) {
		lua_getfield(L, 4, "document_root");
		bind->http_opts.document_root = lua_tostring(L, -1);

		lua_getfield(L, 4, "proto");
		proto = lua_tostring(L, -1);
		
#if MG_ENABLE_SSL	
		lua_getfield(L, 4, "ssl_cert");
		opts.ssl_cert = lua_tostring(L, -1);
	
		lua_getfield(L, 4, "ssl_key");
		opts.ssl_key = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_ca_cert");
		opts.ssl_ca_cert = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_cipher_suites");
		opts.ssl_cipher_suites = lua_tostring(L, -1);
#endif		
	}

	lua_settop(L, 3);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);	
	ctx->callback = ref;
	
	nc = mg_bind_opt(&ctx->mgr, address, ev_handler, opts);
	if (!nc)
		luaL_error(L, "%s", err);

	bind->nc = nc;

	list_add(&bind->node, &ctx->bind_ctx_list);

	if (proto && (!strcmp(proto, "http") || !strcmp(proto, "websocket")))
		mg_set_protocol_http_websocket(nc);

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
#if MG_ENABLE_SSL		
		lua_getfield(L, 4, "ssl_cert");
		opts.ssl_cert = lua_tostring(L, -1);
	
		lua_getfield(L, 4, "ssl_key");
		opts.ssl_key = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_ca_cert");
		opts.ssl_ca_cert = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_cipher_suites");
		opts.ssl_cipher_suites = lua_tostring(L, -1);
#endif		
	}

	lua_settop(L, 3);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	ctx->callback = ref;
	
	nc = mg_connect_opt(&ctx->mgr, address, ev_handler, opts);
	if (!nc) {
		luaL_error(L, "%s", err);
		return 1;
	}

	return 0;
}

static int lua_mg_connect_http(lua_State *L)
{
	int ref;
	struct mg_connection *nc;
	struct mg_connect_opts opts;
	struct mg_context *ctx = luaL_checkudata(L, 1, MONGOOSE_MT);
	const char *url = luaL_checkstring(L, 2);
	const char *extra_headers = NULL;
	const char *post_data = NULL;
	const char *err;
	
	luaL_checktype(L, 3, LUA_TFUNCTION);
	
	memset(&opts, 0, sizeof(opts));

	opts.error_string = &err;
	
	if (lua_istable(L, 4)) {
#if MG_ENABLE_SSL		
		lua_getfield(L, 4, "ssl_cert");
		opts.ssl_cert = lua_tostring(L, -1);
	
		lua_getfield(L, 4, "ssl_key");
		opts.ssl_key = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_ca_cert");
		opts.ssl_ca_cert = lua_tostring(L, -1);
		
		lua_getfield(L, 4, "ssl_cipher_suites");
		opts.ssl_cipher_suites = lua_tostring(L, -1);
#endif
		lua_getfield(L, 4, "extra_headers");
		extra_headers = lua_tostring(L, -1);

		lua_getfield(L, 4, "post_data");
		post_data = lua_tostring(L, -1);
	}

	lua_settop(L, 3);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	ctx->callback = ref;

	nc = mg_connect_http_opt(&ctx->mgr, ev_handler, opts, url, extra_headers, post_data);
	if (!nc) {
		luaL_error(L, "%s", err);
		return 1;
	}

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

static int lua_mg_send_websocket_frame(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 3, &len);
	int op = lua_tointeger(L, 4) || WEBSOCKET_OP_TEXT;
	
	mg_send_websocket_frame(nc, op, buf, len);
	return 0;
}

static int lua_mg_send(lua_State *L)
{
	struct mg_connection *nc = (struct mg_connection *)luaL_checkinteger(L, 2);
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 3, &len);

	mg_send(nc, buf, len);
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
	{"connect_http", lua_mg_connect_http},
	{"set_protocol_mqtt", lua_mg_set_protocol_mqtt},
	{"send_mqtt_handshake_opt", lua_mg_send_mqtt_handshake_opt},	
	{"mqtt_subscribe", lua_mg_mqtt_subscribe},
	{"mqtt_publish", lua_mg_mqtt_publish},
	{"send_websocket_frame", lua_mg_send_websocket_frame},
	{"send", lua_mg_send},
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

	lua_pushinteger(L, MG_EV_WEBSOCKET_HANDSHAKE_DONE);
    lua_setfield(L, -2, "MG_EV_WEBSOCKET_HANDSHAKE_DONE");

	lua_pushinteger(L, MG_EV_WEBSOCKET_FRAME);
    lua_setfield(L, -2, "MG_EV_WEBSOCKET_FRAME");

	lua_pushinteger(L, MG_EV_CONNECT);
    lua_setfield(L, -2, "MG_EV_CONNECT");

	lua_pushinteger(L, MG_EV_HTTP_REPLY);
	lua_setfield(L, -2, "MG_EV_HTTP_REPLY");	

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
	
	lua_pushinteger(L, MG_EV_RECV);
    lua_setfield(L, -2, "MG_EV_RECV");

	lua_pushinteger(L, WEBSOCKET_OP_TEXT);
    lua_setfield(L, -2, "WEBSOCKET_OP_TEXT");
	
	lua_pushinteger(L, WEBSOCKET_OP_BINARY);
    lua_setfield(L, -2, "WEBSOCKET_OP_BINARY");
	
    return 1;
}
