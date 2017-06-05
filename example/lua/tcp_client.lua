#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

-- must be return true
local global_nc = nil
local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_CONNECT then
		print("connect ", msg.connected, msg.err)
		if msg.connected then global_nc = nc end
	elseif event == evmg.MG_EV_RECV then
		print("recv:", msg.data)
	end

	return true
end

mgr:connect("192.168.3.33:8000", ev_handle)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

ev.IO.new(function()
	local data = io.read("*l")
	if global_nc then
		print("send:", data)
		mgr:send(global_nc, data)
	else
		print("Not connected")
	end
end, 0, ev.READ):start(loop)

loop:loop()

mgr:destroy()

print("exit...")

