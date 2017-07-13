#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local sessions = {}

local function generate_sid()
	local t = {}
	for i = 1, 5 do
		t[#t + 1] = string.char(math.random(65, 90))
	end
	
	for i = 1, 5 do
		t[#t + 1] = string.char(math.random(48, 57))
	end
	
	return table.concat(t)
end

local function find_session(sid)
	for _, s in ipairs(sessions) do
		if s.sid == sid then
			s.alive = 6
			return s
		end
	end

	return nil
end

local function del_session(sid)
	for _, s in ipairs(sessions) do
		if s.sid == sid then
			table.remove(sessions, i)
		end
	end

	return nil
end

ev.Timer.new(function(loop, timer, revents)
	for i, s in ipairs(sessions) do
		s.alive = s.alive - 1
		if s.alive == 0 then
			print(s.sid, "timeout")
			table.remove(sessions, i)
		end
	end
end, 5, 5):start(loop)
		
math.randomseed(tostring(os.time()):reverse():sub(1, 6))

local function ev_handle(con, event)
	if event == evmg.MG_EV_HTTP_REQUEST then
		local hm = con:get_evdata()
		
		if hm.uri == "/login.html" then
			if hm.method ~= "POST" then return end

			local user = con:get_http_var("user")
			local pass = con:get_http_var("pass")

			if user and user == "zjh" and pass and pass == "123456" then
				local sid = generate_sid()
				sessions[#sessions + 1] = {sid = sid, alive = 6}
				con:send_http_redirect(302, "/", "Set-Cookie: mgs=" .. sid .. "; path=/");
			else
				con:send_http_redirect(302, "/login.html")
			end
			return true
		end

		local cookie = con:get_http_headers()["Cookie"] or ""
		local sid = cookie:match("mgs=(%w+)")

		if not find_session(sid) then
			con:send_http_redirect(302, "/login.html")
			return true
		end

		if hm.uri == "/logout" then
			del_session(sid)
			con:send_http_redirect(302, "/", "Set-Cookie: mgs=");
			return true
		end
	end
end

local mgr = evmg.init(loop)

mgr:listen(ev_handle, "8000", {proto = "http"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")
