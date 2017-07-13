#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")

-- https://github.com/LuaDist/lzlib
local lz = require "zlib"
local loop = ev.Loop.default
local url = arg[1] or "http://www.baidu.com"

local function ev_handle(con, event)
	if event == evmg.MG_EV_CONNECT then
		--[[
			Detect connection status, If the connection fails, nil is 
			returned and follow an error message is returned,
			Otherwise, return true
		--]]
		print("connection status:", con:connected())
		
	elseif event == evmg.MG_EV_HTTP_REPLY then
		con:set_flags(evmg.MG_F_CLOSE_IMMEDIATELY)
		
		print("resp_code:", con:resp_code())
		print("resp_status_msg:", con:resp_status_msg())

		local headers = con:headers()
		for k, v in pairs(headers) do
			print(k .. ": ", v)
		end

		local body = con:body()

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
