#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

local function ev_handle(nc, event, msg)
end

-- See mongoose.h:4602
mgr:bind("8000", ev_handle, {proto = "http", url_rewrites = "/test=https://www.baidu.com"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")