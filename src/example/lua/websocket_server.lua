#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default
local mgr = evmg.init()

local page = [[
<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8">
	<title>WebSocket Test</title>
	<script>
		var ws = null;
		function connect(btn){
			if (ws) {
				ws.close()
				document.getElementById("btn").value="Connect"
				ws = null;
				return;
			}
			document.getElementById("btn").value = "Disconnect";
			
			ws = new WebSocket("ws://" + location.host);
			ws.onopen = function() {
				console.log("open success");
				ws.send("Hello Evmg");
			};
			
			ws.onmessage = function(evt) {
				console.log("read:" + evt.data);
			};
			
			ws.onclose = function () {
				console.log("server close");
			};
		}
	</script>
	</head>
	<body>
		<input type="button" id="btn" value="Connect" onclick="connect()"/>
	</body>
</html>
]]

local op_str = {
	[evmg.WEBSOCKET_OP_CONTINUE] = "continue",
	[evmg.WEBSOCKET_OP_TEXT] = "text",
	[evmg.WEBSOCKET_OP_BINARY] = "binary",
	[evmg.WEBSOCKET_OP_CLOSE] = "close",
	[evmg.WEBSOCKET_OP_PING] = "ping",
	[evmg.WEBSOCKET_OP_PONG] = "pong",
	[-1] = "unknown"
}

local function ev_handle(nc, event, msg)
	if event == evmg.MG_EV_HTTP_REQUEST then
		mgr:send_head(nc, 200, #page)
		mgr:print(nc, page)
		return true
		
	elseif event == evmg.MG_EV_WEBSOCKET_HANDSHAKE_REQUEST then
		print("method:", msg.method)
		print("uri:", msg.uri)
		print("proto:", msg.proto)
		print("query_string:", msg.query_string)

		for k, v in pairs(msg.headers) do
			print(k, v)
		end

	elseif event == evmg.MG_EV_WEBSOCKET_CONTROL_FRAME then
		print("recv control frame:", op_str[msg.op])
	elseif event == evmg.MG_EV_WEBSOCKET_FRAME then
		print("recv frame", msg.data, op_str[msg.op])
		mgr:send_websocket_frame(nc, "I is evmg", evmg.WEBSOCKET_OP_TEXT)
	end
end

mgr:bind("8000", ev_handle, {proto = "http"})
print("Listen on http 8000...")

ev.Signal.new(function(loop, sig, revents)
	loop:unloop()
end, ev.SIGINT):start(loop)

loop:loop()

mgr:destroy()

print("exit...")
