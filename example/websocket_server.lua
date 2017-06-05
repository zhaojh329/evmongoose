#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmongoose.init(loop)

local function ev_handle(nc, event, msg)
	if event == evmongoose.MG_EV_WEBSOCKET_HANDSHAKE_DONE then
		print("new con:", nc)
	elseif event == evmongoose.MG_EV_WEBSOCKET_FRAME then
		print(msg.data)
		mgr:send_websocket_frame(nc, "I is evmg")
	end
end

mgr:bind("8000", ev_handle, {proto = "websocket"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
