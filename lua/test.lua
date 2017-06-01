#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
local loop = ev.Loop.new()

local function on_timeout()
	print(os.time())
end

local mgr = evmongoose.init(loop)

local function ev_handle()
end

mgr:bind("8000", ev_handle)

loop:loop()

mgr:destroy()