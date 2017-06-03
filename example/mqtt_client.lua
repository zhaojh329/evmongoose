#!/usr/bin/lua

local ev = require("ev")
local evmongoose = require("evmongoose")
--local loop = ev.Loop.default
local loop = ev.Loop.new()

local mgr = evmongoose.init(loop)

local function ev_handle(nc, event, msg)
	if event == evmongoose.MG_EV_CONNECT then
		local opt = {
			user_name = "xxx",
			password = "xxxx",
			client_id = "test123"
		}
		mgr:set_protocol_mqtt(nc)
		mgr:send_mqtt_handshake_opt(nc, opt)
	elseif event == evmongoose.MG_EV_MQTT_CONNACK then
		if msg.connack_ret_code ~= evmongoose.MG_EV_MQTT_CONNACK_ACCEPTED then
			print("Got mqtt connection error:", msg.connack_ret_code)
			return
		end

		local topic = "evmongoose"
		local msg_id = 12

		mgr:mqtt_subscribe(nc, topic, msg_id);
	elseif event == evmongoose.MG_EV_MQTT_SUBACK then
		print("suback, msgid = ", msg.message_id)
		
	elseif event == evmongoose.MG_EV_MQTT_PUBLISH then
		print(msg.topic, msg.payload)
	end
end

mgr:connect("localhost:1883", ev_handle)

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")