#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

-- must be return true
local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_RECV then
		mgr:send(nc, msg.data)
		print("recv:", msg.data)
	end

	return true
end

mgr:bind("8000", ev_handle)
print("Listen on tcp 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
