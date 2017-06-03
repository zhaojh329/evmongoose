#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmongoose.init(loop)

local function ev_handle(nc, msg)
	if msg.uri ~= "/luatest" then
		return false
	end

	mgr:send_head(nc, 200, -1)
	
	mgr:print_http_chunk(nc, "<h1>method:" .. msg.method .. "</h1>")
	mgr:print_http_chunk(nc, "<h1>uri:" .. msg.uri .. "</h1>")
	mgr:print_http_chunk(nc, "<h1>proto:" .. msg.proto .. "</h1>")
	mgr:print_http_chunk(nc, "<h1>query_string:" .. msg.query_string .. "</h1>")

	for k, v in pairs(msg.headers) do
		mgr:print_http_chunk(nc, "<h1>" .. k .. ": " .. v ..  "</h1>")
	end
	
	mgr:print_http_chunk(nc, "")

	return true
end

mgr:bind("8000", ev_handle)

print("Listen on 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")