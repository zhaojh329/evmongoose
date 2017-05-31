#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
local loop = ev.Loop.default

local function on_timeout()
	print(os.time())
end

local mgr = evmongoose.init()
mgr:destroy()

ev.Timer.new(on_timeout, 0.1, 1):start(loop)

loop:loop()