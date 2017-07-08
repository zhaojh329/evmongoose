#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local page = [[<html>
			<body>
				Upload example.
				<form method="POST" action="/upload" enctype="multipart/form-data">
				<input type="file" name="file" /> <br/>
				<input type="submit" value="Upload" />
				</form>
			</body>
			</html>]]

local function ev_handle(con, event)
	if event == evmg.MG_EV_HTTP_REQUEST then
		con:send_http_head(200, #page, "Content-Type: text/html")
		con:send(page)
		return true
	elseif event == evmg.MG_EV_HTTP_MULTIPART_REQUEST_END then
		local fn = con:lfn()
		print("Upload file:", fn)

		local file = io.open(fn, "r")
		local data = file:read(5)
		print(data)
		file:close()
	end
end

local mgr = evmg.init(loop)
local nc = mgr:listen(ev_handle, "8000", {proto = "http"})

print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")
