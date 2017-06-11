#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

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

local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_HTTP_REQUEST then
		if msg.uri == "/login.html" then
			if msg.method ~= "POST" then return end

			local user = mgr:get_http_var(msg.hm, "user")
			local pass = mgr:get_http_var(msg.hm, "pass")

			if user and user == "zjh" and pass and pass == "123456" then
				local sid = generate_sid()
				sessions[#sessions + 1] = {sid = sid, alive = 6}
				mgr:http_send_redirect(nc, 302, "/", "Set-Cookie: mgs=" .. sid .. "; path=/");
			else
				mgr:http_send_redirect(nc, 302, "/login.html")
			end
			return true
		end

		if msg.uri == "/logout" then
			local s = find_session(sid)
			mgr:http_send_redirect(nc, 302, "/", "Set-Cookie: mgs=");
			return true
		end

		local cookie = msg.headers["Cookie"] or ""
		local sid = cookie:match("mgs=(%w+)")

		if not find_session(sid) then
			mgr:http_send_redirect(nc, 302, "/login.html")
			return true
		end

		if msg.uri == "/logout" then
			del_session(sid)
			mgr:http_send_redirect(nc, 302, "/", "Set-Cookie: mgs=");
			return true
		end
	end
end

mgr:bind("8080", ev_handle, {proto = "http"})
print("Listen on http 8080...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

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

loop:loop()

mgr:destroy()

print("exit...")