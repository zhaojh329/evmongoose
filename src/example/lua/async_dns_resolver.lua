#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

local function dns_resolve_cb(domain, ip, err)
	if ip then
		print(domain, "parsed:")
		for _, v in ipairs(ip) do
			print(v)
		end
	else
		print(domain, "parse failed:", err)
	end
	loop:unloop()
end

local domain = arg[1] or "www.baidu.com"

mgr:resolve_async(domain, dns_resolve_cb)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")

