#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local lz = require "zlib"
local loop = ev.Loop.default
local mgr = evmg.init(loop)
local url = arg[1] or "http://www.baidu.com"

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

		local body = msg.body
		if msg.headers["Content-Encoding"] == "gzip" then
			print("Decode Gzip...")
			body = lz.inflate()(body, "finish")
		end
		print("body:")
		print(body)
	end
end

-- Supported opt:
-- extra_headers	Such as "Accept-Encoding: gzip\r\n"
-- debug			If set true, you can deal raw data by MG_EV_RECV
-- post_data		Data to upload
-- ssl_cert
-- ssl_key
-- ssl_ca_cert
-- ssl_cipher_suites
local opt  = {}

mgr:connect_http(url, ev_handle, opt)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
