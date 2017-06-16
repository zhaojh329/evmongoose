#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_WEBSOCKET_HANDSHAKE_REQUEST then
		print("method:", msg.method)
		print("uri:", msg.uri)
		print("proto:", msg.proto)
		print("query_string:", msg.query_string)

		for k, v in pairs(msg.headers) do
			print(k, v)
		end

	elseif event == evmg.MG_EV_WEBSOCKET_FRAME then
		print(msg.data)
		mgr:send_websocket_frame(nc, "I is evmg", evmg.WEBSOCKET_OP_TEXT)
	end
end

mgr:bind("8000", ev_handle, {proto = "http"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
