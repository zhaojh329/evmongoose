#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default


local function ev_handle(nc, event)
end

local mgr = evmg.init(loop)

-- See mongoose.h:4602
mgr:listen(ev_handle, "8000", {proto = "http", url_rewrites = "/test=https://www.baidu.com"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")