#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_CONNECT then
		print("connect ", msg.connected, msg.err)
	elseif event == evmg.MG_EV_HTTP_REPLY then
		mgr:set_connection_flags(nc, evmg.MG_F_CLOSE_IMMEDIATELY)
		
		print("resp_code:", msg.resp_code)
		print("resp_status_msg:", msg.resp_status_msg)
		for k, v in pairs(msg.headers) do
			print(k .. ": ", v)
		end

		print("body:", msg.body)
	end
end

mgr:connect_http("http://www.baidu.com", ev_handle)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
