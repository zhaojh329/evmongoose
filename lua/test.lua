#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()


local mgr = evmongoose.init(loop)

local function ev_handle(nc, msg)
	local content = "lua for evmongoose"
	
	print("method:", msg.method)
	print("uri:", msg.uri)
	print("proto:", msg.proto)
	print("query_string:", msg.query_string)
	
	mgr:printf(nc, "HTTP/1.1 200 OK\r\n")
	mgr:printf(nc, "Content-Length: " .. #content .. "\r\n\r\n")
	mgr:printf(nc, content)
end

mgr:bind("8000", ev_handle)

loop:loop()

mgr:destroy()