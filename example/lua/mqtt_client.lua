#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

local broker = arg[1] or "localhost:1883"
local keep_alive = 3

local alive_timer = ev.Timer.new(function(loop, w, event)
	keep_alive = keep_alive - 1
	if keep_alive == 0 then w:stop(loop) end
end, 5, 5)

local function ev_handle(con, event)
	if event == evmg.MG_EV_CONNECT then
		local result = con:get_evdata()
		if not result.connected then
			print("connect failed:", result.err)
			return
		end

		--[[
			Supported opt:
			user_name
			password
			client_id:		Default is: "evmongoose" .. mg_time()
			clean_session:	Default is false
			will_retain:	Default is false
		--]]
		
		local opt = {clean_session = true}
		con:mqtt_handshake(opt)
		
	elseif event == evmg.MG_EV_MQTT_CONNACK then
		local msg = con:get_evdata()
		
		if msg.code ~= evmg.MG_EV_MQTT_CONNACK_ACCEPTED then
			print("mqtt connection failed:", msg.err)
			return
		else
			print("mqtt connection ok")
		end

		local topic = {
			{name = "topic1"},
			{name = "topic2", qos = 1}
		}
		local mid = 12

		con:mqtt_subscribe(topic, mid)
		alive_timer:start(loop)

	elseif event == evmg.MG_EV_MQTT_SUBACK then
		local msg = con:get_evdata()
		print("conack mid:", msg.mid)
		
	elseif event == evmg.MG_EV_MQTT_PUBLISH then
		local msg = con:get_evdata()
		print("topic:", msg.topic)
		print("payload:", msg.payload)
		print("qos:", msg.qos)
		print("msgid:", msg.mid)

		con:mqtt_publish("test", "12345678")
		
	elseif event == evmg.MG_EV_MQTT_PINGRESP then
		print("Recv PingResp")
		keep_alive = 3

	elseif event == evmg.MG_EV_POLL then
		if keep_alive == 0 then
			-- Disconnect and reconnection
			con:set_flags(evmg.MG_F_CLOSE_IMMEDIATELY)
			keep_alive = 3
		end
	elseif event == evmg.MG_EV_CLOSE then
		print("connection close")
		
		ev.Timer.new(function()
			print("Try Reconnect to", broker)
			local mgr = con:get_mgr()
			mgr:connect(ev_handle, broker)
		end, 5):start(loop)
	end
end

local mgr = evmg.init(loop)
mgr:connect(ev_handle, broker)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

print("exit...")
