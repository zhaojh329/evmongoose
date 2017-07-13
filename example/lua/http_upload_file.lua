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
	elseif event == evmg.MG_EV_HTTP_PART_BEGIN then
		local part = con:get_evdata()
		print("upload begin")
		print("var_name:", part.var_name)
		print("file_name:", part.file_name)

		local file_name = part.file_name
		if not file_name or #file_name == 0 or file_name:match("[^%w%-%._]+") then
			con:send("HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nInvalid filename");

			-- If the false is returned, it continues to be processed by the underlying layer, otherwise finish end processing
			return true
		end
	elseif event == evmg.MG_EV_HTTP_PART_END then
		local part = con:get_evdata()
		print("upload finish")
		print("var_name:", part.var_name)
		print("file_name:", part.file_name)
		print("local file name:", part.lfn)

		if not part.lfn then
			print("Upload file failed")
		else
			-- Now you can handle the file at will
			os.rename(part.lfn, "/tmp/test_" .. part.file_name)
			os.remove("/tmp/test_" .. part.file_name)
		end
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
