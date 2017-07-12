/* Lua wrapper for the frequently-used posix C API */

#include "lua_mongoose.h"

#include <syslog.h>
#include <pty.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

static int l_openlog(lua_State *L) 
{
	const char *ident = luaL_checkstring(L, 1);
	int option = luaL_checkint(L, 2);
	int facility = luaL_checkint(L, 3);

	openlog(ident, option, facility);
	return 0;
}

static int l_syslog(lua_State *L)
{
	int priority = luaL_checkint(L, 1);
	const char *msg = luaL_checkstring(L, 2);

	syslog(priority, "%s", msg);
	return 0;
}

static int l_closelog(lua_State *L)
{
	closelog();
	return 0;
}

static int lua_mg_getopt_iter(lua_State *L)
{
	int opt, longindex;
	const char *optstring = luaL_checkstring(L, lua_upvalueindex(1));
	int argc = lua_tointeger(L, lua_upvalueindex(2));
	char **argv = (char **)lua_touserdata(L, lua_upvalueindex(3));
	struct option *longopts = (struct option *)lua_touserdata(L, lua_upvalueindex(4));;
	
	opt = getopt_long(argc, argv, optstring, longopts, &longindex);
	if (opt == -1)
		return 0;
	
	lua_pushlstring(L, (char *)&opt, 1);
	lua_pushstring(L, optarg);
	lua_pushinteger(L, longindex + 1);
	
	return 3;
}

static int lua_mg_getopt(lua_State *L)
{
	int i, argc, n = 0;
	char **argv;
	struct option *longopts;

	luaL_checkstring(L, 2);
	luaL_checktype(L, 1, LUA_TTABLE);

	argc = lua_objlen(L, 1) + 1;
	lua_pushinteger(L, argc);
	
	argv = lua_newuserdata(L, argc * sizeof(char *));
	
	for (i = 0; i < argc; i++) {
		lua_rawgeti(L, 1, i);
		argv[i] = (char *)luaL_checkstring(L, -1);
		lua_pop(L, 1);
	}

	if (lua_type(L, 3) == LUA_TTABLE)
		n = lua_objlen(L, 3);

	longopts = lua_newuserdata(L, (n + 1) * sizeof(struct option));
	
	longopts[n].name = NULL;
	longopts[n].has_arg = 0;
	longopts[n].flag = NULL;
	longopts[n].val = 0;

	for (i = 1; i <= n; i++) {
		const char *name, *val;
		int has_arg;
		
		lua_rawgeti(L, 3, i);
		luaL_checktype(L, -1, LUA_TTABLE);

		lua_rawgeti(L, -1, 1);
		name = luaL_checkstring(L, -1);
		lua_pop(L, 1);
		
		lua_rawgeti(L, -1, 2);
		has_arg = lua_toboolean(L, -1);
		lua_pop(L, 1);
		
		lua_rawgeti(L, -1, 3);
		val = luaL_checkstring(L, -1);
		lua_pop(L, 2);
		
		longopts[i - 1].name = name;
		longopts[i - 1].has_arg = has_arg;
		longopts[i - 1].flag = NULL;
		longopts[i - 1].val = val[0];
	}

	if (lua_type(L, 3) == LUA_TTABLE)
		lua_remove(L, 3);

	lua_pushcclosure(L, lua_mg_getopt_iter, 4);
	return 1;
}

static int lua_forkpty(lua_State *L)
{
	pid_t pid;
	int pty;
	
	if (lua_gettop(L)) {
		struct termios t;
			
		luaL_checktype(L, 1, LUA_TTABLE);
		
		memset(&t, 0, sizeof(t));
		
		lua_getfield(L, 1, "iflag"); t.c_iflag = luaL_optinteger(L, -1, 0);
		lua_getfield(L, 1, "oflag"); t.c_oflag = luaL_optinteger(L, -1, 0);
		lua_getfield(L, 1, "cflag"); t.c_cflag = luaL_optinteger(L, -1, 0);
		lua_getfield(L, 1, "lflag"); t.c_lflag = luaL_optinteger(L, -1, 0);
		
		lua_getfield(L, 1, "cc");
		if (!lua_isnoneornil(L, -1)) {
			luaL_checktype(L, -1, LUA_TTABLE);
			for (int i = 0; i < NCCS; i++) {
				lua_pushinteger(L, i);
				lua_gettable(L, -2);
				t.c_cc[i] = luaL_optinteger(L, -1, 0);
				lua_pop(L, 1);
			}
		}
		pid = forkpty(&pty, NULL, &t, NULL);
	} else {
		pid = forkpty(&pty, NULL, NULL, NULL);
	}
	
	if (pid < 0) 
		luaL_error(L, strerror(errno));

	lua_pushinteger(L, pid);
	lua_pushinteger(L, pty);
	
	return 2;
}

static int lua_access(lua_State *L)
{
	const char *pathname = luaL_checkstring(L, 1);
	const char *s = lua_tostring(L, 2);
	int mode = F_OK;

	if (s) {
		if (strchr(s, 'r'))
			mode |= R_OK;

		if (strchr(s, 'w'))
			mode |= W_OK;

		if (strchr(s, 'x'))
			mode |= X_OK;
	}
	lua_pushinteger(L, access(pathname, mode));

	return 1;
}

static int lua_read(lua_State *L)
{
	int fd = luaL_checkinteger(L, 1);
	int count = luaL_checkinteger(L, 2);
	int ret;
	char *buf = NULL;

	buf = malloc(count);
	if (!buf)
		goto err;
	
	ret = read(fd, buf, count);
	if (ret < 0)
		goto err;

	lua_pushlstring(L, buf, ret);
	free(buf);
	return 1;
	
err:
	if (buf)
		free(buf);
	
	lua_pushnil(L);
	lua_pushstring(L, strerror(errno));
	return 2;
}

static int lua_write(lua_State *L)
{
	int fd = luaL_checkinteger(L, 1);
	size_t count;
	const char *buf = luaL_checklstring(L, 2, &count);
	
	count = write(fd, buf, count);
	if (count < 0) {
		lua_pushstring(L, strerror(errno));
		lua_pushnil(L);
		return 2;
	}

	lua_pushinteger(L, count);
	return 1;	
}

static int lua_run_exec(lua_State *L, int use_shell)
{
	char **argv;
	const char *path = luaL_checkstring(L, 1);
	int i, n, ret;

	luaL_checktype(L, 2, LUA_TTABLE);

	n = lua_objlen(L, 2);
	argv = lua_newuserdata(L, (n + 1) * sizeof(char*));

	/* Read argv from table. */
	for (i = 0; i < n; i++) {
		lua_rawgeti(L, 2, i + 1);
		argv[i] = (char *)lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	argv[n] = NULL;

	if (use_shell)
		ret = execvp(path, argv);
	else
		ret = execv(path, argv);
	if (ret < 0) {
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		return 2;
	}

	lua_pushinteger(L, 0);
	return 1;
}

static int lua_exec(lua_State *L)
{
	return lua_run_exec(L, 0);
}

static int lua_execp(lua_State *L)
{
	return lua_run_exec(L, 1);
}

static int lua_wait(lua_State *L)
{
	pid_t pid = luaL_checkinteger(L, 1);
	int options = lua_tointeger(L, 2);
	int status;
	
	pid = waitpid(pid, &status, options);
	if (pid < 0) {
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		return 2;
	}

	lua_pushinteger(L, pid);
	
	if (WIFEXITED(status)) {
		lua_pushliteral(L,"exited");
		lua_pushinteger(L, WEXITSTATUS(status));
		return 3;
	} else if (WIFSIGNALED(status)) {
		lua_pushliteral(L,"killed");
		lua_pushinteger(L, WTERMSIG(status));
		return 3;
	} else if (WIFSTOPPED(status)) {
		lua_pushliteral(L,"stopped");
		lua_pushinteger(L, WSTOPSIG(status));
		return 3;
	}
	
	return 1;
}

static int lua_kill(lua_State *L)
{
	pid_t pid = luaL_checkinteger(L, 1);
	int sig = luaL_checkinteger(L, 2);

	if (kill(pid, sig) < 0) {
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		return 2;
	}

	lua_pushinteger(L, 0);
	return 1;
}

static int lua_fork(lua_State *L)
{
	pid_t pid = fork();
	if (pid < 0) {
		lua_pushnil(L);
		lua_pushstring(L, strerror(errno));
		return 2;
	}
	
	lua_pushinteger(L, pid);
	return 1;
}

static const struct luaL_Reg R[] = {
	{"openlog", l_openlog},
	{"syslog", l_syslog},
	{"closelog", l_closelog},
	{"getopt", lua_mg_getopt},
	{"forkpty", lua_forkpty},
	{"access", lua_access},
	{"read", lua_read},
	{"write", lua_write},
	{"exec", lua_exec},
	{"execp", lua_execp},
	{"wait", lua_wait},
	{"kill", lua_kill},
	{"fork", lua_fork},
	{NULL, NULL}
};

int luaopen_evmongoose_posix(lua_State *L)
{
	lua_newtable(L);
	luaL_register(L, NULL, R);

	/* The option argument to openlog() */
	EVMG_LUA_SETCONST(LOG_CONS);
	EVMG_LUA_SETCONST(LOG_NDELAY);
	EVMG_LUA_SETCONST(LOG_NOWAIT);
	EVMG_LUA_SETCONST(LOG_ODELAY);
	EVMG_LUA_SETCONST(LOG_PERROR);
	EVMG_LUA_SETCONST(LOG_PID);

	/* The facility argument to openlog() */
	EVMG_LUA_SETCONST(LOG_AUTH);
	EVMG_LUA_SETCONST(LOG_AUTHPRIV);
	EVMG_LUA_SETCONST(LOG_CRON);
	EVMG_LUA_SETCONST(LOG_DAEMON);
	EVMG_LUA_SETCONST(LOG_FTP);
	EVMG_LUA_SETCONST(LOG_KERN);
	EVMG_LUA_SETCONST(LOG_LOCAL0);
	EVMG_LUA_SETCONST(LOG_LOCAL1);
	EVMG_LUA_SETCONST(LOG_LOCAL2);
	EVMG_LUA_SETCONST(LOG_LOCAL3);
	EVMG_LUA_SETCONST(LOG_LOCAL4);
	EVMG_LUA_SETCONST(LOG_LOCAL5);
	EVMG_LUA_SETCONST(LOG_LOCAL6);
	EVMG_LUA_SETCONST(LOG_LOCAL7);
	EVMG_LUA_SETCONST(LOG_LPR);
	EVMG_LUA_SETCONST(LOG_MAIL);
	EVMG_LUA_SETCONST(LOG_NEWS);
	EVMG_LUA_SETCONST(LOG_SYSLOG);
	EVMG_LUA_SETCONST(LOG_USER);
	EVMG_LUA_SETCONST(LOG_UUCP);

	/* The level argument to syslog() */
	EVMG_LUA_SETCONST(LOG_EMERG);
	EVMG_LUA_SETCONST(LOG_ALERT);
	EVMG_LUA_SETCONST(LOG_CRIT);
	EVMG_LUA_SETCONST(LOG_ERR);
	EVMG_LUA_SETCONST(LOG_WARNING);
	EVMG_LUA_SETCONST(LOG_NOTICE);
	EVMG_LUA_SETCONST(LOG_INFO);
	EVMG_LUA_SETCONST(LOG_DEBUG);

	/* options for wait */
	EVMG_LUA_SETCONST(WNOHANG);
	EVMG_LUA_SETCONST(WUNTRACED);
	EVMG_LUA_SETCONST(WCONTINUED);

	/* signum for kill */
	EVMG_LUA_SETCONST(SIGABRT);
	EVMG_LUA_SETCONST(SIGALRM);
	EVMG_LUA_SETCONST(SIGBUS);
	EVMG_LUA_SETCONST(SIGCHLD);
	EVMG_LUA_SETCONST(SIGCONT);
	EVMG_LUA_SETCONST(SIGFPE);
	EVMG_LUA_SETCONST(SIGHUP);
	EVMG_LUA_SETCONST(SIGINT);
	EVMG_LUA_SETCONST(SIGIO);
	EVMG_LUA_SETCONST(SIGIOT);
	EVMG_LUA_SETCONST(SIGKILL);
	EVMG_LUA_SETCONST(SIGPIPE);
#ifdef SIGPOLL
	EVMG_LUA_SETCONST(SIGPOLL);
#endif
	EVMG_LUA_SETCONST(SIGPROF);
#ifdef SIGPWR
	EVMG_LUA_SETCONST(SIGPWR);
#endif
	EVMG_LUA_SETCONST(SIGQUIT);
	EVMG_LUA_SETCONST(SIGSEGV);
#ifdef SIGSTKFLT
	EVMG_LUA_SETCONST(SIGSTKFLT);
#endif
	EVMG_LUA_SETCONST(SIGSYS);
	EVMG_LUA_SETCONST(SIGTERM);
	EVMG_LUA_SETCONST(SIGTRAP);
	EVMG_LUA_SETCONST(SIGTSTP);
	EVMG_LUA_SETCONST(SIGTTIN);
	EVMG_LUA_SETCONST(SIGTTOU);
	EVMG_LUA_SETCONST(SIGURG);
	EVMG_LUA_SETCONST(SIGUSR1);
	EVMG_LUA_SETCONST(SIGUSR2);
	EVMG_LUA_SETCONST(SIGVTALRM);
	EVMG_LUA_SETCONST(SIGWINCH);
	EVMG_LUA_SETCONST(SIGXCPU);
	EVMG_LUA_SETCONST(SIGXFSZ);

	return 1;
}

