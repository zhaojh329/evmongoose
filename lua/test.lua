#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()


local mgr = evmongoose.init(loop)

local function ev_handle(nc)
	mgr:printf(nc, "HTTP/1.1 200 OK\r\n")
	mgr:printf(nc, "Content-Length: 3\r\n\r\n")
	mgr:printf(nc, "lua for evmongoose")
end

mgr:bind("8000", ev_handle)

loop:loop()

mgr:destroy()