#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local function ev_handle(con, event)
	if event ~= evmg.MG_EV_HTTP_REQUEST then return end

	local hm = con:get_evdata()
	if hm.uri ~= "/proxy" then return end

	--See mongoose.h:4866
	con:http_reverse_proxy("/proxy", "http://www.baidu.com")

	return true
end

local mgr = evmg.init(loop)
mgr:listen(ev_handle, "8000", {proto = "http"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")
