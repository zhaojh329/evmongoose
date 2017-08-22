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

#include "lua_mongoose.h"
#include "mongoose.h"

#define LOOP_MT    "ev{loop}"
#define UNINITIALIZED_DEFAULT_LOOP (struct ev_loop *)1
#define EVMONGOOSE_MT "evmongoose"
#define EVMONGOOSE_CON_MT "evmongoose{con}"
#define EVMONGOOSE_DNS_MT "evmongoose{dns}"

#define EVMG_F_IS_MQTT		(1 << 0)

static char *obj_registry = "evmongoose{obj}";

struct lua_mg_connection {
	struct mg_mgr *mgr;
	struct mg_connection *con;
	struct mg_connection *con2;	/* Store Accepted con */
	struct mg_serve_http_opts http_opts;
	int event;
	void *ev_data;
	unsigned flags;
};

struct lua_mg_dns_ctx {
	lua_State *L;
	const char *domain;
};

#if 0
static void lua_print_stack(lua_State *L, const char *info)
{
	int i = 1;
	printf("----------%s----------\n", info);

	for (; i <= lua_gettop(L); i++) {
		printf("%d %s\n", i, lua_typename(L, lua_type(L, i)));
	}
}
#endif

/**
 * Taken from lua.c out of the lua source distribution.  Use this
 * function when doing lua_pcall().
 */
static int traceback(lua_State *L) {
    if ( !lua_isstring(L, 1) ) return 1;

    lua_getglobal(L, "debug");
    if ( !lua_istable(L, -1) ) {
        lua_pop(L, 1);
        return 1;
    }

    lua_getfield(L, -1, "traceback");
    if ( !lua_isfunction(L, -1) ) {
        lua_pop(L, 2);
        return 1;
    }

    lua_pushvalue(L, 1);    /* pass error message */
    lua_pushinteger(L, 2);  /* skip this function and traceback */
    lua_call(L, 2, 1);      /* call debug.traceback */
    return 1;
}

/**
 * Create a "registry" of light userdata pointers into the
 * fulluserdata so that we can get handles into the lua objects.
 */
static void create_obj_registry(lua_State *L)
{
    lua_pushlightuserdata(L, &obj_registry);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

/**
 * Count the number of registered objects.  This exists only so we can
 * validate that objects are properly GC'ed.
 *
 * [-0, +1, e]
 */
static int obj_count(lua_State *L) {
    int count = 0;

    lua_pushlightuserdata(L, &obj_registry);
    lua_rawget(L,            LUA_REGISTRYINDEX);
    assert(lua_istable(L, -1) /* create_obj_registry() should have ran */);

    lua_pushnil(L);
    while ( lua_next(L, -2) != 0 ) {
        count++;
        lua_pop(L, 1);
    }
    lua_pushinteger(L, count);
    return 1;
}

static void lua_obj_registry(lua_State* L, void *obj)
{	
	lua_pushlightuserdata(L, &obj_registry);
    lua_rawget(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, obj);
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
}

static void lua_con_obj_init(lua_State* L, void *obj)
{
	luaL_checktype(L, 2, LUA_TFUNCTION);
	
	lua_createtable(L, 1, 0);
	lua_pushvalue(L, 2);
    lua_rawseti(L, -2, 1);
	lua_setuservalue(L, -2);
}

static void *lua_obj_new(lua_State* L, size_t size, const char *tname)
{	
	void *obj = lua_newuserdata(L, size);
	luaL_getmetatable(L, tname);
	lua_setmetatable(L, -2);
    return obj;
}

static void lua_obj_del(lua_State* L, void *obj)
{
	lua_pushlightuserdata(L, &obj_registry);
	lua_rawget(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, obj);
	lua_pushnil(L);
	lua_rawset(L, -3);
}

static void lua_mg_ev_handler(struct mg_connection *con, int event, void *ev_data)
{
	lua_State *L = (lua_State *)con->mgr->user_data;
	struct lua_mg_connection *lcon;
	int ret;
	
	if (con->listener) {
		lcon = (struct lua_mg_connection *)con->listener->user_data;
		lcon->con2 = con;
	} else {
		lcon = (struct lua_mg_connection *)con->user_data;
		lcon->con2 = NULL;
	}

	lcon->event = event;
	lcon->ev_data = ev_data;
	
	lua_pushcfunction(L, traceback);
	
	lua_pushlightuserdata(L, &obj_registry);
	lua_rawget(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, lcon);
	lua_rawget(L, -2);

	lua_getuservalue(L, -1);
	lua_rawgeti(L, -1, 1);

	lua_insert(L, -3);
	lua_pop(L, 1);

	lua_pushinteger(L, event);

	if (event == MG_EV_HTTP_PART_END) {
		struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *)ev_data;
		struct file_upload_state *fus = (struct file_upload_state *)mp->user_data;

		con->flags |= MG_F_SEND_AND_CLOSE;
		
		if (fus) {
			if (mp->status >= 0 && fus->fp) {
				mg_printf(con, "HTTP/1.1 200 OK\r\n"
								"Content-Type: text/plain\r\n"
								"Connection: close\r\n\r\n"
								"Ok, %s - %d bytes.\r\n",
								mp->file_name, (int)fus->num_recd);
			}else {
				/* mp->status < 0 means connection was terminated, so no reason to send HTTP reply */
			}

			if (fus->fp) fclose(fus->fp);
		}
	}
	
	if (lua_pcall(L, 2, 1, -5) ) {
		/* TODO: Enable user-specified error handler! */
		fprintf(stderr, "CALLBACK FAILED: %s\n", lua_tostring(L, -1));
		return;
	}

	ret = lua_toboolean(L, -1);
	
	switch (event) {
	case MG_EV_HTTP_REQUEST:
		if (con->listener && !ret)
			mg_serve_http(con, ev_data, lcon->http_opts);		/* Serve static content */
		break;

	case MG_EV_HTTP_PART_BEGIN:
		if (!ret) {
			struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *)ev_data;
			struct file_upload_state *fus = calloc(1, sizeof(struct file_upload_state));

			mp->user_data = NULL;
			fus->lfn = calloc(1, strlen("/tmp/") + strlen(mp->file_name) + 1);
			strcpy(fus->lfn, "/tmp/");
			strcpy(fus->lfn + strlen("/tmp/"), mp->file_name);

			fus->fp = fopen(fus->lfn, "w");
			if (fus->fp == NULL) {
				mg_printf(con, "HTTP/1.1 500 Internal Server Error\r\n"
								"Content-Type: text/plain\r\n"
								"Connection: close\r\n\r\n");
				mg_printf(con, "Failed to open %s: %d\n", fus->lfn, errno);
				/* Do not close the connection just yet, discard remainder of the data.
				* This is because at the time of writing some browsers (Chrome) fail to
				* render response before all the data is sent. */
			}
			mp->user_data = (void *) fus;
		}
		break;
	case MG_EV_HTTP_PART_DATA: {
		struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *)ev_data;
		struct file_upload_state *fus = (struct file_upload_state *)mp->user_data;

		if (!fus || !fus->fp) break;
		if (fwrite(mp->data.p, 1, mp->data.len, fus->fp) != mp->data.len) {
			if (errno == ENOSPC) {
				mg_printf(con, "HTTP/1.1 413 Payload Too Large\r\n"
								"Content-Type: text/plain\r\n"
								"Connection: close\r\n\r\n");
				mg_printf(con, "Failed to write to %s: no space left; wrote %d\r\n", fus->lfn, (int)fus->num_recd);
			} else {
				mg_printf(con, "HTTP/1.1 500 Internal Server Error\r\n"
                   	 			"Content-Type: text/plain\r\n"
                    			"Connection: close\r\n\r\n");
          		mg_printf(con, "Failed to write to %s: %d, wrote %d", mp->file_name, errno, (int)fus->num_recd);
			}
			fclose(fus->fp);
			remove(fus->lfn);
			fus->fp = NULL;
			/* Do not close the connection just yet, discard remainder of the data.
			 * This is because at the time of writing some browsers (Chrome) fail to
			 * render response before all the data is sent. */
			return;
		}
		fus->num_recd += mp->data.len;
		break;
	}
	case MG_EV_HTTP_PART_END: {
		struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *)ev_data;
		struct file_upload_state *fus = (struct file_upload_state *)mp->user_data;

		if (fus) {
			if (fus->fp)
				remove(fus->lfn);
			free(fus->lfn);
			free(fus);
			mp->user_data = NULL;
		}
		break;
	}
	case MG_EV_CLOSE:
		if (!con->listener)
			lua_obj_del(L, lcon);
		break;
	}		

	lua_settop(L, 0);
}

/**************************** meta function of mg_mgr ********************************/
static int lua_mg_connect(lua_State *L)
{	
	struct mg_connection *con;
	struct lua_mg_connection *lcon;
	const char *address = lua_tostring(L, 3);
	struct mg_connect_opts opts;
	const char *err;
	
	lcon = (struct lua_mg_connection *)lua_obj_new(L, sizeof(struct lua_mg_connection), EVMONGOOSE_CON_MT);
	lcon->mgr = luaL_checkudata(L, 1, EVMONGOOSE_MT);
	lua_con_obj_init(L, lcon);
	lua_obj_registry(L, lcon);

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

		lua_pop(L, 4);
#endif
	}

	lua_pop(L, 1);
	
	con = mg_connect_opt(lcon->mgr, address, lua_mg_ev_handler, opts);
	if (!con)
		return luaL_error(L, "%s", err);
	
	con->user_data = lcon;
	lcon->con = con;
	
	return 1;
}

static int lua_mg_connect_http(lua_State *L)
{	
	struct mg_connection *con;
	struct lua_mg_connection *lcon;
	const char *url = luaL_checkstring(L, 3);
	struct mg_connect_opts opts;
	const char *extra_headers = NULL;
	const char *post_data = NULL;
	const char *err;
	
	lcon = (struct lua_mg_connection *)lua_obj_new(L, sizeof(struct lua_mg_connection), EVMONGOOSE_CON_MT);
	lcon->mgr = luaL_checkudata(L, 1, EVMONGOOSE_MT);
	lua_con_obj_init(L, lcon);
	lua_obj_registry(L, lcon);
	
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

		lua_pop(L, 4);
#endif
		lua_getfield(L, 4, "extra_headers");
		extra_headers = lua_tostring(L, -1);

		lua_getfield(L, 4, "post_data");
		post_data = lua_tostring(L, -1);

		lua_pop(L, 2);
	}
	
	lua_pop(L, 1);
	
	con = mg_connect_http_opt(lcon->mgr, lua_mg_ev_handler, opts, url, extra_headers, post_data);
	if (!con)
		return luaL_error(L, "%s", err);

	con->user_data = lcon;
	lcon->con = con;
	
	return 1;
}

static int lua_mg_listen(lua_State *L)
{
	struct mg_connection *con;
	struct lua_mg_connection *lcon;
	const char *address = lua_tostring(L, 3);
	struct mg_bind_opts opts;
	const char *proto = NULL;
	const char *err = NULL;
	
	lcon = (struct lua_mg_connection *)lua_obj_new(L, sizeof(struct lua_mg_connection), EVMONGOOSE_CON_MT);
	lcon->mgr = luaL_checkudata(L, 1, EVMONGOOSE_MT);
	lua_con_obj_init(L, lcon);
	lua_obj_registry(L, lcon);

	memset(&lcon->http_opts, 0, sizeof(struct mg_serve_http_opts));
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

		lua_pop(L, 4);
#endif
		lua_getfield(L, 4, "proto");
		proto = lua_tostring(L, -1);

		lua_pop(L, 1);
		
		if (proto && !strcmp(proto, "http")) {
			lua_getfield(L, 4, "document_root");
			lcon->http_opts.document_root = lua_tostring(L, -1);

			lua_getfield(L, 4, "index_files");
			lcon->http_opts.index_files = lua_tostring(L, -1);

			lua_getfield(L, 4, "enable_directory_listing");
			if (!lua_toboolean(L, -1))
				lcon->http_opts.enable_directory_listing = "no";
				
			lua_getfield(L, 4, "url_rewrites");
			lcon->http_opts.url_rewrites = lua_tostring(L, -1);

			lua_pop(L, 4);
		}
	}

	lua_pop(L, 1);
	
	con = mg_bind_opt(lcon->mgr, address, lua_mg_ev_handler, opts);
	if (!con)
		return luaL_error(L, "%s", err);

	con->user_data = lcon;
	lcon->con = con;

	if (proto && !strcmp(proto, "http"))
		mg_set_protocol_http_websocket(con);
	
	return 1;
}

static void dns_resolve_cb(struct mg_dns_message *msg, void *data, enum mg_resolve_err e)
{
	struct lua_mg_dns_ctx *dns = (struct lua_mg_dns_ctx *)data;
	lua_State *L = dns->L;
	int i = 1;
	struct in_addr ina;
	struct mg_dns_resource_record *rr = NULL;
	
	lua_pushcfunction(L, traceback);
	
	lua_pushlightuserdata(L, &obj_registry);
	lua_rawget(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, dns);
	lua_rawget(L, -2);

	lua_getuservalue(L, -1);
	lua_rawgeti(L, -1, 1);

	lua_insert(L, -3);
	lua_pop(L, 1);

	lua_pushstring(L, dns->domain);

	if (!msg || e != MG_RESOLVE_OK) {
		lua_pushnil(L);
		
		switch (e) {
		case MG_RESOLVE_NO_ANSWERS:
			lua_pushstring(L, "No answers");
			break;
		case MG_RESOLVE_EXCEEDED_RETRY_COUNT:
			lua_pushstring(L, "Exceeded retry count");
			break;
		case MG_RESOLVE_TIMEOUT:
			lua_pushstring(L, "Timeout");
			break;
		default:
			lua_pushstring(L, "Unknown error");
			break;
		}
		goto ret;
	}

	lua_newtable(L);
	
	while (1) {
		rr = mg_dns_next_record(msg, MG_DNS_A_RECORD, rr);
		if (!rr)
			break;

		if (mg_dns_parse_record_data(msg, rr, &ina, sizeof(ina)))
			break;

		lua_pushstring(L, inet_ntoa(ina));
		lua_rawseti(L, -2, i++);
	}

	lua_pushstring(L, "ok");
	
ret:	
	if (lua_pcall(L, 4, 0, -7)) {
		/* TODO: Enable user-specified error handler! */
		fprintf(stderr, "CALLBACK FAILED: %s\n", lua_tostring(L, -1));
	}
	
	lua_obj_del(L, dns);
	lua_settop(L, 0);
}

static int lua_mg_resolve_async(lua_State *L)
{
	struct lua_mg_dns_ctx *dns;
	const char *domain = lua_tostring(L, 3);
	struct mg_resolve_async_opts opts;
	
	dns =(struct lua_mg_dns_ctx *)lua_obj_new(L, sizeof(struct lua_mg_dns_ctx), EVMONGOOSE_DNS_MT);
	lua_con_obj_init(L, dns);
	lua_obj_registry(L, dns);
	
	dns->domain = domain;
	dns->L = L;
	
	memset(&opts, 0, sizeof(opts));
	
	if (lua_istable(L, 4)) {		
		lua_getfield(L, 4, "max_retries");
		opts.max_retries = lua_tointeger(L, -1);

		lua_getfield(L, 4, "timeout");
		opts.timeout = lua_tointeger(L, -1);

		lua_getfield(L, 4, "nameserver");
		opts.nameserver_url = lua_tostring(L, -1);
	}

	mg_resolve_async_opt(luaL_checkudata(L, 1, EVMONGOOSE_MT), domain, MG_DNS_A_RECORD, dns_resolve_cb, dns, opts);
	return 0;
}

/**************************** meta function of mg_connection ********************************/
static int lua_get_mgr(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_mgr *mgr = lcon->mgr;

	lua_pushlightuserdata(L, &obj_registry);
	lua_rawget(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, mgr);
	lua_rawget(L, -2);

	return 1;
}

static int lua_mg_set_flags(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	
	con->flags |= luaL_checkinteger(L, 2);
	return 0;
}

/* Returns the corresponding data according to the corresponding event type */
static int lua_mg_get_evdata(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	int event = lcon->event;

	lua_newtable(L);
				
	switch (event) {
	case MG_EV_CONNECT: {
			int err = *(int *)lcon->ev_data;

			lua_pushboolean(L, !err);
			lua_setfield(L, -2, "connected");
				
			lua_pushstring(L, strerror(err));
			lua_setfield(L, -2, "err");

			break;
		}
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
	case MG_EV_HTTP_REQUEST: {
			struct http_message *hm = (struct http_message *)lcon->ev_data;

			lua_pushlstring(L, hm->uri.p, hm->uri.len);
			lua_setfield(L, -2, "uri");

			lua_pushlstring(L, hm->method.p, hm->method.len);
			lua_setfield(L, -2, "method");

			lua_pushlstring(L, hm->proto.p, hm->proto.len);
			lua_setfield(L, -2, "proto");

			lua_pushlstring(L, hm->query_string.p, hm->query_string.len);
			lua_setfield(L, -2, "query_string");

			lua_pushstring(L, inet_ntoa(con->sa.sin.sin_addr));
			lua_setfield(L, -2, "remote_addr");
			
			break;
		}
	case MG_EV_HTTP_REPLY: {
			struct http_message *hm = (struct http_message *)lcon->ev_data;

			lua_pushinteger(L, hm->resp_code);
			lua_setfield(L, -2, "status_code");

			lua_pushlstring(L, hm->resp_status_msg.p, hm->resp_status_msg.len);
			lua_setfield(L, -2, "status_msg");
			
			break;
		}
	case MG_EV_WEBSOCKET_CONTROL_FRAME:
	case MG_EV_WEBSOCKET_FRAME: {
			struct websocket_message *wm = (struct websocket_message *)lcon->ev_data;

			if (wm->flags & WEBSOCKET_OP_CONTINUE)
				lua_pushinteger(L, WEBSOCKET_OP_CONTINUE);
			else if (wm->flags & WEBSOCKET_OP_TEXT)
				lua_pushinteger(L, WEBSOCKET_OP_TEXT);
			else if (wm->flags & WEBSOCKET_OP_BINARY)
				lua_pushinteger(L, WEBSOCKET_OP_BINARY);
			else if (wm->flags & WEBSOCKET_OP_CLOSE)
				lua_pushinteger(L, WEBSOCKET_OP_CLOSE);
			else if (wm->flags & WEBSOCKET_OP_PING)
				lua_pushinteger(L, WEBSOCKET_OP_PING);
			else if (wm->flags & WEBSOCKET_OP_PONG)
				lua_pushinteger(L, WEBSOCKET_OP_PONG);
			else
				lua_pushinteger(L, -1);
			
			lua_setfield(L, -2, "op");

			if (event == MG_EV_WEBSOCKET_FRAME) {
				lua_pushlstring(L, (const char *)wm->data, wm->size);
				lua_setfield(L, -2, "frame");
			}
	
			break;
		}
	case MG_EV_HTTP_PART_BEGIN:
	case MG_EV_HTTP_PART_END: {
			struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *)lcon->ev_data;
			struct file_upload_state *fus = (struct file_upload_state *)mp->user_data;

			lua_pushstring(L, mp->var_name);
			lua_setfield(L, -2, "var_name");

			lua_pushstring(L, mp->file_name);
			lua_setfield(L, -2, "file_name");

			if (fus && mp->status >= 0 && fus->fp) {
				lua_pushstring(L, fus->lfn);
				lua_setfield(L, -2, "lfn");
			}
			break;
		}
	case MG_EV_MQTT_CONNACK: {
			struct mg_mqtt_message *msg = (struct mg_mqtt_message *)lcon->ev_data;

			lua_pushinteger(L, msg->connack_ret_code);
			lua_setfield(L, -2, "code");

			switch (msg->connack_ret_code) {
			case MG_EV_MQTT_CONNACK_ACCEPTED:
				lua_pushstring(L, "Connection Accepted");
				break;
			case MG_EV_MQTT_CONNACK_UNACCEPTABLE_VERSION:
				lua_pushstring(L, "Connection Refused: unacceptable protocol version");
				break;
			case MG_EV_MQTT_CONNACK_IDENTIFIER_REJECTED:
				lua_pushstring(L, "Connection Refused: identifier rejected");
				break;
			case MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE:
				lua_pushstring(L, "Connection Refused: server unavailable");
				break;
			case MG_EV_MQTT_CONNACK_BAD_AUTH:
				lua_pushstring(L, "Connection Refused: bad user name or password");
				break;
			case MG_EV_MQTT_CONNACK_NOT_AUTHORIZED:
				lua_pushstring(L, "Connection Refused: not authorized");
				break;
			default:
				lua_pushstring(L, "Unknown Error");
				break;
			}
			lua_setfield(L, -2, "err");
			
			break;
		}
	case MG_EV_MQTT_SUBACK: {
			struct mg_mqtt_message *msg = (struct mg_mqtt_message *)lcon->ev_data;
			lua_pushinteger(L, msg->message_id);
			lua_setfield(L, -2, "mid");
			break;
		}
	case MG_EV_MQTT_PUBLISH: {
			struct mg_mqtt_message *msg = (struct mg_mqtt_message *)lcon->ev_data;

			lua_pushlstring(L, msg->topic.p, msg->topic.len);
			lua_setfield(L, -2, "topic");
			
			lua_pushlstring(L, msg->payload.p, msg->payload.len);
			lua_setfield(L, -2, "payload");
			
			lua_pushinteger(L, msg->qos);
			lua_setfield(L, -2, "qos");
			
			lua_pushinteger(L, msg->message_id);
			lua_setfield(L, -2, "mid");
			break;
		}
	case MG_EV_MQTT_PUBACK: {
			struct mg_mqtt_message *msg = (struct mg_mqtt_message *)lcon->ev_data;
			lua_pushinteger(L, msg->message_id);
			lua_setfield(L, -2, "mid");
			break;
		}
	}

	return 1;
}

/* Get raw data from connection */
static int lua_mg_recv(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	struct mbuf *io = &con->recv_mbuf;

	lua_pushlstring(L, io->buf, io->len);
	mbuf_remove(io, io->len);

	return 1;
}

static int lua_mg_sent_size(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);

	lua_pushinteger(L, *(int *)lcon->ev_data);
	return 1;
}

static int lua_mg_send(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 2, &len);

	mg_send(con, buf, len);
	return 0;
}

static int lua_mg_send_http_chunk(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 2, &len);

	mg_send_http_chunk(con, buf, len);
	return 0;
}

static int lua_mg_send_http_head(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	int status_code = luaL_checkinteger(L, 2);
	int content_length = luaL_checkinteger(L, 3);
	const char *extra_headers = lua_tostring(L, 4);

	mg_send_head(con, status_code, content_length, extra_headers);
	return 0;
}

static int lua_mg_send_http_redirect(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	int status_code = luaL_checkinteger(L, 2);
	const char *location = luaL_checkstring(L, 3);
	const char *extra_headers = lua_tostring(L, 4);

	if (status_code != 301 && status_code != 302)
		luaL_error(L, "\"status_code\" should be either 301 or 302");
	
	mg_http_send_redirect(con, status_code, mg_mk_str(location), mg_mk_str(extra_headers));
	return 0;
}

static int lua_mg_send_http_error(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	int code = luaL_checkinteger(L, 2);
	const char *reason = lua_tostring(L, 3);
	
	mg_http_send_error(con, code, reason);
	return 0;
}

static int lua_mg_http_reverse_proxy(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	struct http_message *hm = (struct http_message *)lcon->ev_data;
	const char *mount = luaL_checkstring(L, 2);
	const char *upstream = luaL_checkstring(L, 3);

	mg_http_reverse_proxy(con, hm, mg_mk_str(mount), mg_mk_str(upstream));
	return 0;
}

static int lua_mg_get_http_headers(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct http_message *hm = (struct http_message *)lcon->ev_data;
	int i;
	char tmp[128];

	lua_newtable(L);

	for (i = 0; hm->header_names[i].len > 0; i++) {
		struct mg_str *h = &hm->header_names[i];
		struct mg_str *v = &hm->header_values[i];
		if (h->p) {
			lua_pushlstring(L, v->p, v->len);
			snprintf(tmp, sizeof(tmp), "%.*s", (int)h->len, h->p);
			lua_setfield(L, -2, tmp);
		}
	}
	return 1;
}

static int lua_mg_get_http_body(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct http_message *hm = (struct http_message *)lcon->ev_data;

	lua_pushlstring(L, hm->body.p, hm->body.len);
	return 1;
}

static int lua_mg_get_http_var(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct http_message *hm = (struct http_message *)lcon->ev_data;
	const char *name = luaL_checkstring(L, 2);
	char value[64] = "";

	if (mg_get_http_var(&hm->query_string, name, value, sizeof(value)) > 0)
		lua_pushstring(L, value);
	else if (mg_get_http_var(&hm->body, name, value, sizeof(value)) > 0)
		lua_pushstring(L, value);
	else
		lua_pushnil(L);

	return 1;
}

static int lua_mg_is_websocket(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;

	if (con->flags & MG_F_IS_WEBSOCKET)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}

static int lua_mg_send_websocket_frame(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	size_t len = 0;
	const char *buf = luaL_checklstring(L, 2, &len);
	int op = luaL_checkinteger(L, 3);

	mg_send_websocket_frame(con, op, buf, len);
	return 0;
}

static int lua_mg_is_mqtt(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);

	if (lcon->flags & EVMG_F_IS_MQTT)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}

static int lua_mg_mqtt_handshake(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	struct mg_send_mqtt_handshake_opts opts;
	char client_id[128] = "";

	memset(&opts, 0, sizeof(opts));
	sprintf(client_id, "evmongoose:%f", mg_time());

	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "user_name");
		opts.user_name = lua_tostring(L, -1);

		lua_getfield(L, 2, "password");
		opts.password = lua_tostring(L, -1);

		lua_getfield(L, 2, "client_id");
		if (lua_tostring(L, -1))
			strncpy(client_id, lua_tostring(L, -1), sizeof(client_id));

		lua_getfield(L, 2, "clean_session");
		if (lua_toboolean(L, -1))
			opts.flags |= MG_MQTT_CLEAN_SESSION;

		lua_getfield(L, 2, "will_retain");
		if (lua_toboolean(L, -1))
			opts.flags |= MG_MQTT_WILL_RETAIN;
	}
	
	mg_set_protocol_mqtt(con);
	mg_send_mqtt_handshake_opt(con, client_id, opts);

	lcon->flags |= EVMG_F_IS_MQTT;

	return 0;
}

static int lua_mg_mqtt_subscribe(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	struct mg_mqtt_topic_expression *topic_expr;
	int i, topics_len, mid;
	
	luaL_checktype(L, 2, LUA_TTABLE);

	topics_len = lua_rawlen(L, 2);

	if (topics_len > 0)
		topic_expr = calloc(topics_len, sizeof(struct mg_mqtt_topic_expression));

	for (i = 0; i < topics_len; i++) {	
		lua_rawgeti(L, 2, i + 1);
		luaL_checktype(L, -1, LUA_TTABLE);

		lua_getfield(L, -1, "name");
		topic_expr[i].topic = luaL_checkstring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, -1, "qos");
		topic_expr[i].qos = lua_tointeger(L, -1);
		lua_pop(L, 2);
	}
	
	mid = lua_tointeger(L, 3);

	if (topics_len > 0) {
		mg_mqtt_subscribe(con, topic_expr, topics_len, mid);
		free(topic_expr);
	}
	
	return 0;
}

static int lua_mg_mqtt_publish(lua_State *L)
{
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);
	struct mg_connection *con = lcon->con2 ? lcon->con2 : lcon->con;
	const char *topic = luaL_checkstring(L, 2);
	size_t len = 0;
	const char *payload = luaL_checklstring(L, 3, &len);
	int mid = lua_tointeger(L, 4);
	int qos = lua_tointeger(L, 5);
	
	mg_mqtt_publish(con, topic, mid, MG_MQTT_QOS(qos), payload, len);
	return 0;
}


static int lua_mg_con_tostring(lua_State *L)
{
	char buf[32];
	struct lua_mg_connection *lcon = luaL_checkudata(L, 1, EVMONGOOSE_CON_MT);

	sprintf(buf, "%p", lcon);
	lua_pushstring(L, buf);
	
	return 1;
}

/*************************evmongoose global function*******************************************/
static int lua_mg_mgr_init(lua_State *L)
{
	struct ev_loop *loop = NULL;
	struct mg_mgr *mgr = lua_obj_new(L, sizeof(struct mg_mgr), EVMONGOOSE_MT);
	lua_obj_registry(L, mgr);
	lua_pop(L, 1);

	if (lua_gettop(L) > 1) {
		struct ev_loop **tmp = luaL_checkudata(L, 1, LOOP_MT);
		if (*tmp != UNINITIALIZED_DEFAULT_LOOP)
			loop = *tmp;
	}
	mg_mgr_init(mgr, L, loop);

	return 1;
}

static int lua_mg_mgr_free(lua_State *L)
{
	struct mg_mgr *mgr = luaL_checkudata(L, 1, EVMONGOOSE_MT);
	lua_obj_del(L, mgr);
	mg_mgr_free(mgr);
	return 0;
}

static int lua_mg_time(lua_State *L)
{
	lua_pushnumber(L, mg_time());
	return 1;
}

static void __lua_mg_md5(lua_State *L, unsigned char *hash)
{
	MD5_CTX ctx;
	int i, nargs;
	size_t len;
	const char *data;

	MD5_Init(&ctx);

	nargs = lua_gettop(L);

	for (i = 0; i < nargs; i++) {
		data = luaL_checklstring(L, i + 1, &len);
		MD5_Update(&ctx, (const unsigned char *)data, len);
	}

	MD5_Final(hash, &ctx);
}

static int lua_mg_md5(lua_State *L)
{
	unsigned char hash[16];

	__lua_mg_md5(L, hash);
	lua_pushlstring(L, (const char *)hash, 16);
	
	return 1;
}

static int lua_mg_cs_md5(lua_State *L)
{
	unsigned char hash[16];
	char cs[33] = "";

	__lua_mg_md5(L, hash);
	cs_to_hex(cs, hash, sizeof(hash));
	lua_pushstring(L, cs);
	
	return 1;
}

static const luaL_Reg evmongoose_con_meta[] = {
	{"get_mgr", lua_get_mgr},
	{"set_flags", lua_mg_set_flags},
	{"get_evdata", lua_mg_get_evdata},
	{"recv", lua_mg_recv},
	{"sent_size", lua_mg_sent_size},
	{"send", lua_mg_send},
	{"send_http_chunk", lua_mg_send_http_chunk},
	{"send_http_head", lua_mg_send_http_head},
	{"send_http_redirect", lua_mg_send_http_redirect},
	{"send_http_error", lua_mg_send_http_error},
	{"http_reverse_proxy", lua_mg_http_reverse_proxy},
	{"get_http_headers", lua_mg_get_http_headers},
	{"get_http_body", lua_mg_get_http_body},
	{"get_http_var", lua_mg_get_http_var},
	{"is_websocket", lua_mg_is_websocket},
	{"send_websocket_frame", lua_mg_send_websocket_frame},
	{"is_mqtt", lua_mg_is_mqtt},
	{"mqtt_handshake", lua_mg_mqtt_handshake},
	{"mqtt_subscribe", lua_mg_mqtt_subscribe},
	{"mqtt_publish", lua_mg_mqtt_publish},
	{"tostring", lua_mg_con_tostring},
	{NULL, NULL}
};

static const luaL_Reg evmongoose_dns_meta[] = {
	{NULL, NULL}
};

static const luaL_Reg evmongoose_meta[] = {
	{"connect", lua_mg_connect},
	{"connect_http", lua_mg_connect_http},
	{"listen", lua_mg_listen},
	{"dns_resolve_async", lua_mg_resolve_async},
	{"object_count", obj_count},
	{"__gc", lua_mg_mgr_free},
	{NULL, NULL}
};

static const luaL_Reg evmongoose_fun[] = {
	{"init", lua_mg_mgr_init},
	{"mg_time", lua_mg_time},
	{"md5", lua_mg_md5},
	{"cs_md5", lua_mg_cs_md5},
	{NULL, NULL}
};

int luaopen_evmongoose(lua_State *L) 
{
	create_obj_registry(L);
	
	/* metatable.__index = metatable */
    luaL_newmetatable(L, EVMONGOOSE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, evmongoose_meta, 0);

	luaL_newmetatable(L, EVMONGOOSE_CON_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, evmongoose_con_meta, 0);

	luaL_newmetatable(L, EVMONGOOSE_DNS_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, evmongoose_dns_meta, 0);

	lua_newtable(L);
	luaL_setfuncs(L, evmongoose_fun, 0);

	luaopen_evmongoose_posix(L);
	lua_setfield(L, -2, "posix");

	EVMG_LUA_SETCONST(MG_F_SEND_AND_CLOSE);		/* Push remaining data and close  */
	EVMG_LUA_SETCONST(MG_F_CLOSE_IMMEDIATELY);	/* Disconnect */
	
	EVMG_LUA_SETCONST(MG_EV_POLL);    /* Sent to each connection on 1s interval */
	EVMG_LUA_SETCONST(MG_EV_ACCEPT);  /* New connection accepted. union socket_address * */
	EVMG_LUA_SETCONST(MG_EV_CONNECT); /* connect() succeeded or failed. int *  */
	EVMG_LUA_SETCONST(MG_EV_RECV);    /* Data has benn received. int *num_bytes */
	EVMG_LUA_SETCONST(MG_EV_SEND);    /* Data has been written to a socket. int *num_bytes */
	EVMG_LUA_SETCONST(MG_EV_CLOSE);   /* Connection is closed. NULL */
	EVMG_LUA_SETCONST(MG_EV_TIMER);   /* now >= conn->ev_timer_time. double * */

	EVMG_LUA_SETCONST(MG_EV_HTTP_REQUEST);
	EVMG_LUA_SETCONST(MG_EV_HTTP_REPLY);
	EVMG_LUA_SETCONST(MG_EV_HTTP_CHUNK);

	EVMG_LUA_SETCONST(MG_EV_HTTP_MULTIPART_REQUEST);
	EVMG_LUA_SETCONST(MG_EV_HTTP_PART_BEGIN);
	EVMG_LUA_SETCONST(MG_EV_HTTP_PART_DATA);
	EVMG_LUA_SETCONST(MG_EV_HTTP_PART_END);
	EVMG_LUA_SETCONST(MG_EV_HTTP_MULTIPART_REQUEST_END);

	EVMG_LUA_SETCONST(WEBSOCKET_OP_CONTINUE);
	EVMG_LUA_SETCONST(WEBSOCKET_OP_TEXT);
	EVMG_LUA_SETCONST(WEBSOCKET_OP_BINARY);
	EVMG_LUA_SETCONST(WEBSOCKET_OP_CLOSE);
	EVMG_LUA_SETCONST(WEBSOCKET_OP_PING);
	EVMG_LUA_SETCONST(WEBSOCKET_OP_PONG);

	EVMG_LUA_SETCONST(MG_EV_WEBSOCKET_HANDSHAKE_REQUEST);
	EVMG_LUA_SETCONST(MG_EV_WEBSOCKET_HANDSHAKE_DONE);
	EVMG_LUA_SETCONST(MG_EV_WEBSOCKET_FRAME);
	EVMG_LUA_SETCONST(MG_EV_WEBSOCKET_CONTROL_FRAME);

	/* see http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html */
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNECT);		/* Client requests a connection to a server */
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK);		/* Acknowledge connection request */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PUBLISH);		/* Publish message */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PUBACK);		/* Publish acknowledgment */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PUBREC);		/* Assured publish received  */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PUBREL);		/* Assured Publish Release */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PUBCOMP);		/* Assured publish complete */
	EVMG_LUA_SETCONST(MG_EV_MQTT_SUBSCRIBE);	/* Subscribe to named topics */
	EVMG_LUA_SETCONST(MG_EV_MQTT_SUBACK);		/* Subscription acknowledgement */
	EVMG_LUA_SETCONST(MG_EV_MQTT_UNSUBSCRIBE);	/* Unsubscribe from named topics */
	EVMG_LUA_SETCONST(MG_EV_MQTT_UNSUBACK);		/* Unsubscribe acknowledgment */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PINGREQ);		/* PING request */
	EVMG_LUA_SETCONST(MG_EV_MQTT_PINGRESP);		/* PING response */
	EVMG_LUA_SETCONST(MG_EV_MQTT_DISCONNECT);	/* Disconnect notification */

	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK_ACCEPTED);
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK_UNACCEPTABLE_VERSION);
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK_IDENTIFIER_REJECTED);
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE);
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK_BAD_AUTH);
	EVMG_LUA_SETCONST(MG_EV_MQTT_CONNACK_NOT_AUTHORIZED);
	
    return 1;
}
