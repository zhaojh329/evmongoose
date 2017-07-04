/* Lua wrapper for the syslog C API
 * See syslog(3)
 */

#include <syslog.h>
#include "lua_mongoose.h"
		
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

static const struct luaL_Reg R[] = {
	{"openlog", l_openlog},
	{"syslog", l_syslog},
	{"closelog", l_closelog},
	{NULL, NULL}
};

int luaopen_evmongoose_syslog(lua_State *L)
{
	lua_newtable(L);
	luaL_register(L, NULL, R);

	/* The option argument to openlog() */
	EVMG_LUA_ADD_VARIABLE(LOG_CONS);
	EVMG_LUA_ADD_VARIABLE(LOG_NDELAY);
	EVMG_LUA_ADD_VARIABLE(LOG_NOWAIT);
	EVMG_LUA_ADD_VARIABLE(LOG_ODELAY);
	EVMG_LUA_ADD_VARIABLE(LOG_PERROR);
	EVMG_LUA_ADD_VARIABLE(LOG_PID);

	/* The facility argument to openlog() */
	EVMG_LUA_ADD_VARIABLE(LOG_AUTH);
	EVMG_LUA_ADD_VARIABLE(LOG_AUTHPRIV);
	EVMG_LUA_ADD_VARIABLE(LOG_CRON);
	EVMG_LUA_ADD_VARIABLE(LOG_DAEMON);
	EVMG_LUA_ADD_VARIABLE(LOG_FTP);
	EVMG_LUA_ADD_VARIABLE(LOG_KERN);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL0);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL1);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL2);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL3);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL4);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL5);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL6);
	EVMG_LUA_ADD_VARIABLE(LOG_LOCAL7);
	EVMG_LUA_ADD_VARIABLE(LOG_LPR);
	EVMG_LUA_ADD_VARIABLE(LOG_MAIL);
	EVMG_LUA_ADD_VARIABLE(LOG_NEWS);
	EVMG_LUA_ADD_VARIABLE(LOG_SYSLOG);
	EVMG_LUA_ADD_VARIABLE(LOG_USER);
	EVMG_LUA_ADD_VARIABLE(LOG_UUCP);

	/* The level argument to syslog() */
	EVMG_LUA_ADD_VARIABLE(LOG_EMERG);
	EVMG_LUA_ADD_VARIABLE(LOG_ALERT);
	EVMG_LUA_ADD_VARIABLE(LOG_CRIT);
	EVMG_LUA_ADD_VARIABLE(LOG_ERR);
	EVMG_LUA_ADD_VARIABLE(LOG_WARNING);
	EVMG_LUA_ADD_VARIABLE(LOG_NOTICE);
	EVMG_LUA_ADD_VARIABLE(LOG_INFO);
	EVMG_LUA_ADD_VARIABLE(LOG_DEBUG);
	
	return 1;
}

