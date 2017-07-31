#!/usr/bin/lua

local evmg = require("evmongoose")
local posix = evmg.posix

posix.openlog("evmg.posix", posix.LOG_ODELAY + posix.LOG_PERROR, posix.LOG_USER)

local function logger(level, ...)
	posix.syslog(level, table.concat({...}, "  "))
end

logger(posix.LOG_INFO, "syslog", "test")
posix.closelog()

-- Sleep 1s
posix.usleep(1000 * 1000)

-- run in the background
-- posix.daemon()

-- f: whether the file exists
-- r: whether the file exists and grants read permissions
-- w: whether the file exists and grants write permissions
-- x: whether the file exists and grants execute permissions
local r = posix.access("/tmp/test.txt", "frwx")
print("access:", r)

-- read data from stdin
local input = posix.read(0, 1024)

-- write data to stdout
posix.write(1, "You Inputed:" .. input)

local pid = posix.fork()
if pid == 0 then
	posix.exec("/bin/ls", {"/tmp"})
	
	-- Use shell
	--posix.execp("ls", {"/tmp"})
else
	print("In parent")
	os.execute("sleep 1")
	posix.kill(pid, posix.SIGKILL)
	print(posix.wait(pid))
end


