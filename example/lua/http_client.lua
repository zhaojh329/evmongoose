#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")

-- https://github.com/LuaDist/lzlib
local lz = require "zlib"
local loop = ev.Loop.default
local url = arg[1] or "http://www.baidu.com"

local function ev_handle(con, event)
	if event == evmg.MG_EV_CONNECT then
		local result = con:get_evdata()
		print("connection result:", result.connected, result.err)
		
	elseif event == evmg.MG_EV_HTTP_REPLY then
		con:set_flags(evmg.MG_F_CLOSE_IMMEDIATELY)
		local hm = con:get_evdata()
		
		print("resp_code:", hm.status_code)
		print("resp_status_msg:", hm.status_msg)

		local headers = con:get_http_headers()
		for k, v in pairs(headers) do
			print(k .. ": ", v)
		end

		local body = con:get_http_body()

		if headers["Content-Encoding"] == "gzip" then
			print("Decode Gzip...")
			local stream = lz.inflate(body)
			body = stream:read("*a")
			stream:close()
		end

		print("body:", body)
	end
end

local mgr = evmg.init(loop)

-- Supported opt:
-- extra_headers	Such as "Accept-Encoding: gzip\r\n"
-- post_data
-- ssl_cert
-- ssl_key
-- ssl_ca_cert
-- ssl_cipher_suites
local opt  = {extra_headers = "User-Agent:Evmongoose\r\nAccept-Encoding: gzip\r\n"}
mgr:connect_http(ev_handle, url, opt)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")
