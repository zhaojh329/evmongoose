#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)


local page = [[<html>
			<body>
				Upload example.
				<form method="POST" action="/upload" enctype="multipart/form-data">
				<input type="file" name="file" /> <br/>
				<input type="submit" value="Upload" />
				</form>
			</body>
			</html>]]

local upfile

--return a path for store the upload file
--other Not allow upload
local function upload_fname(fname)
	upfile = "/tmp/" .. fname
	return upfile
end

local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_HTTP_REQUEST then
		mgr:send_head(nc, 200, #page, "Content-Type: text/html")
		mgr:print(nc, page)
		return true
	elseif event == evmg.MG_EV_HTTP_MULTIPART_REQUEST_END then
		if upfile then
			print("Upload file path:", upfile)
			
			local file = io.open(upfile, "r")
			if file then
				local data = file:read("*a")
				print("Upload file size:", #data)
				print("Now delete " .. upfile)
				os.execute("rm " .. upfile)
				file:close()
			end
		end
	end
end

local nc = mgr:bind("8000", ev_handle, {proto = "http"})
mgr:set_fu_fname_fn(nc, upload_fname)

print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
