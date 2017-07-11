#!/usr/bin/lua

local evmg = require("evmongoose")

local long_options = {
	{"host", true, "h"},
	{"port", true, "p"},
	{"help", false, "0"},
	{"verbose", false, "0"}
}

local function usage(pro)
	print(pro, "[option]")
	print([[
	-h,--host          host to connect to
	-p, --port         Port to connect to
	-d                 Turn on debug
	--help             show help
	--verbose          Make the operation more talkative
	]])
	
	os.exit();
end

for opt, optarg, longindex in evmg.getopt(arg, "h:p:d") do
	if opt == "h" then
		print("host:", optarg)
	elseif opt == "p" then
		print("port:", optarg)
	elseif opt == "d" then
		print("debug: on")
	elseif opt == "0" then
		local name = long_options[longindex][1]			
		if name == "verbose" then
			print("verbose: on")
		end
	else
		usage(arg[0])
	end
end