#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local function ev_handle(con, event)
	if event == evmg.MG_EV_RECV then
		local data = con:recv()
		con:send(data)
		print("recv:", data)
	end
end

local mgr = evmg.init(loop)

mgr:listen(ev_handle, "udp://8000")
print("Listen on udp 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")

