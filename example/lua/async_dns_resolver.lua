#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local mgr = evmg.init(loop)

local function dns_resolve_cb(ctx, domain, ip, err)
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

-- max_retries: defaults to 5
-- timeout:		in seconds; defaults to 5
-- nameserver:	"udp://114.114.114.114:53";
mgr:dns_resolve_async(dns_resolve_cb, domain, {max_retries = 1, timeout = 2})

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")

