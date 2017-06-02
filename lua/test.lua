#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
local loop = ev.Loop.new()

local function on_timeout()
	print(os.time())
end

local mgr = evmongoose.init(loop)

local function ev_handle(nc)
	mgr:printf(nc, "HTTP/1.1 200 OK\r\n")
	mgr:printf(nc, "Content-Length: 3\r\n\r\n")
	mgr:printf(nc, "123")
end

mgr:bind("8000", ev_handle)

loop:loop()

mgr:destroy()