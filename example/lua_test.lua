#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmongoose.init(loop)

local function ev_handle(nc, msg)
	mgr:send_head(nc, 200, -1)
	
	mgr:print_http_chunk(nc, "<h1>method:" .. msg.method .. "</h1>")
	mgr:print_http_chunk(nc, "<h1>uri:" .. msg.uri .. "</h1>")
	mgr:print_http_chunk(nc, "<h1>proto:" .. msg.proto .. "</h1>")
	mgr:print_http_chunk(nc, "<h1>query_string:" .. msg.query_string .. "</h1>")
	mgr:print_http_chunk(nc, "")
end

mgr:bind("8000", ev_handle)

print("Listen on 8000...")

loop:loop()

mgr:destroy()
