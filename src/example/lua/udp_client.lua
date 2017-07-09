#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local function ev_handle(con, event)
	if event == evmg.MG_EV_CONNECT then
		con:send("Hello, I'm evmongoose")
		
	elseif event == evmg.MG_EV_RECV then
		local data = con:recv()
		con:send(data)
		print("recv:", data)
	end
end

local mgr = evmg.init(loop)

mgr:connect(ev_handle, "udp://192.168.0.101:8080")
print("Connect to udp://192.168.0.101:8080...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")

