#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmongoose.init(loop)

local function dns_resolve_cb(domain, ip)
	print(domain, "parsed:")

	for _, v in ipairs(ip) do
		print(v)
	end

	loop:unloop()
end

mgr:resolve_async("www.baidu.com", dns_resolve_cb)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")

