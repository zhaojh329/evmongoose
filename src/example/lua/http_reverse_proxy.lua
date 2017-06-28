#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default
local mgr = evmg.init()

local function ev_handle(nc, event, msg)
	if event ~= evmg.MG_EV_HTTP_REQUEST or msg.uri ~= "/proxy" then
		return false
	end

	--See mongoose.h:4872
	mgr:http_reverse_proxy(nc, msg.hm, "/proxy", "http://www.baidu.com")

	return true
end

mgr:bind("8000", ev_handle, {proto = "http"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
