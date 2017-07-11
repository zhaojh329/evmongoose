#!/usr/bin/lua

local evmg = require("evmongoose")

local longopts = {
	{"host", true, "h"},	-- required_argument
	{"port", true, "p"},	-- required_argument
	{"help", false, 0},		-- no_argument
}

print("option", "optarg", "long-option")
for o, optarg, lo in evmg.getopt(arg, "h:p:dt:", longopts) do
	print(o, optarg, lo)
end