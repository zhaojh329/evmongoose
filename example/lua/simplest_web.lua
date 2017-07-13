#!/usr/bin/lua

local ev = require("ev")
-- https://github.com/LuaDist/lzlib
local lz = require("zlib")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local function ev_handle(con, event)
	if event ~= evmg.MG_EV_HTTP_REQUEST then return end

	-- Fetch http message
	local hm = con:get_evdata()
	local uri = hm.uri

	print("uri:", uri)

	if uri ~= "/luatest" then return end

	print("method:", hm.method)
	print("proto:", hm.proto)
	print("query_string:", hm.query_string)
	print("remote_addr:", hm.remote_addr)

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

	local rsp = "Hello, I'm Evmongoose"

	local chunk = true
	if chunk then
		con:send_http_head(200, -1)
		con:send_http_chunk(rsp)
		con:send_http_chunk("")
	else
		con:send_http_head(200, #rsp)
		con:send(rsp)
	end

	-- Must return true, if dealed by user
	return true
end

local mgr = evmg.init(loop)

-- Supported opt:
-- proto						Must be set to "http" for a http or https server, also includes websocket server
-- document_root				Default is "."
-- index_files					Default is "index.html,index.htm,index.shtml,index.cgi,index.php"
-- enable_directory_listing		Default if false
mgr:listen(ev_handle, "8000", {proto = "http", enable_directory_listing = true})
print("Listen on http 8000...")

mgr:listen(ev_handle, "7443", {proto = "http", ssl_cert = "server.pem", ssl_key = "server.key"})
print("Listen on https 7443...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")
