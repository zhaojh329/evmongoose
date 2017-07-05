#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmg.init(loop)

local alive = 3

local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_CONNECT then
		local opt = {
			user_name = "xxx",
			password = "xxxx",
			client_id = "test123",
			clean_session = true,
			will_retain = false	--Default is false
		}
		mgr:set_protocol_mqtt(nc)
		mgr:send_mqtt_handshake_opt(nc, opt)
		
	elseif event == evmg.MG_EV_MQTT_CONNACK then
		print("mqtt connection:", nc, msg.code, msg.reason)
					
		if msg.code ~= evmg.MG_EV_MQTT_CONNACK_ACCEPTED then return end

		local topic = "evmongoose"
		local msg_id = 12

		mgr:mqtt_subscribe(nc, topic, msg_id);
		
	elseif event == evmg.MG_EV_MQTT_SUBACK then
		print("suback, msgid = ", msg.message_id)
		
	elseif event == evmg.MG_EV_MQTT_PUBLISH then
		print(msg.topic, msg.payload)
		mgr:mqtt_publish(nc, "test", "12345678")
		
	elseif event == evmg.MG_EV_MQTT_PINGRESP then
		print("Recv PingResp:", nc)
		alive = 5

	elseif event == evmg.MG_EV_POLL then
		if alive == 0 then
			-- Disconnect and reconnection
			mgr:set_connection_flags(nc, evmg.MG_F_CLOSE_IMMEDIATELY)
		end
	elseif event == evmg.MG_EV_CLOSE then
		print("connection close:", nc)
		
		ev.Timer.new(function()
			print("Try Reconnect...")
			mgr:connect("localhost:1883", ev_handle)
		end, 2):start(loop)
	end
end

mgr:connect("localhost:1883", ev_handle)

ev.Timer.new(function()
	alive = alive - 1
end, 5, 5):start(loop)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
