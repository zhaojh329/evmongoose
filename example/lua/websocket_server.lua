#!/usr/bin/lua

local ev = require("ev")
local evmg = require("evmongoose")
local loop = ev.Loop.default

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

local function ev_handle(con, event)
	if event == evmg.MG_EV_HTTP_REQUEST then
		con:send_http_head(200, #page)
		con:send(page)
		
	elseif event == evmg.MG_EV_WEBSOCKET_HANDSHAKE_REQUEST then
		-- Fetch http message
		local hm = con:get_evdata()
	
		print("method:", hm.method)
		print("proto:", hm.proto)
		print("query_string:", hm.query_string)
		print("remote_addr:", hm.remote_addr)

		local headers = con:get_http_headers()
			for k, v in pairs(headers) do
			print(k .. ": ", v)
		end

	elseif event == evmg.MG_EV_WEBSOCKET_CONTROL_FRAME then
		local msg = con:get_evdata()
		print("recv control frame:", op_str[msg.op])
		
	elseif event == evmg.MG_EV_WEBSOCKET_FRAME then
		local msg = con:get_evdata()
		print("recv frame", msg.frame, op_str[msg.op])
		con:send_websocket_frame("I is evmg", evmg.WEBSOCKET_OP_TEXT)
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
