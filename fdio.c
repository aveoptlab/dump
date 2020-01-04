#include "lua_head.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/select.h>

#define MAXERRMSGLEN 256

static void panic_error(lua_State *L, char *msg)
{
    char msgbuf[MAXERRMSGLEN];
    strerror_r(errno, msgbuf, MAXERRMSGLEN);
    if (msg)
	luaL_error(L, msg, msgbuf);
    else
	luaL_error(L, msgbuf);
}

LUAFN(status)
{
    int in_fd = -1, out_fd = -1;
    fd_set in_set, out_set;
    int nfd = 0;
    FD_ZERO(&in_set);
    FD_ZERO(&out_set);
    if (lua_toboolean(L, 1)) {
	in_fd = luaL_checkinteger(L, 1);
	FD_SET(in_fd, &in_set);
	nfd = in_fd+1;
    }
    if (lua_toboolean(L, 2)) {
	out_fd = luaL_checkinteger(L, 2);
	FD_SET(out_fd, &out_set);
        if (out_fd > in_fd)
	    nfd = out_fd + 1;
    }
    int use_timeout = !lua_isnoneornil(L, 3);
    struct timeval timeout;
    if (use_timeout) {
	double time = luaL_checknumber(L, 3);
	timeout.tv_sec = trunc(time);
	timeout.tv_usec = 1E6 * (time - timeout.tv_sec);
    }

    if (select(nfd+1, &in_set, &out_set, NULL,
	       use_timeout ? &timeout : NULL) < 0)
	panic_error(L, "termios select failure: %s");

    lua_pushboolean(L, in_fd >= 0 ? FD_ISSET(in_fd, &in_set) : 0);
    lua_pushboolean(L, out_fd >= 0 ? FD_ISSET(out_fd, &out_set) : 0);
    return 2;
}

LUAFN(blocking)
{
    int fd = luaL_checkinteger(L, 1);
    int mode = fcntl(fd, F_GETFL);
    if (mode == -1)
	panic_error(L, "blocking fcntl get failure: %s");
    if (lua_isnone(L, 2)) {
	lua_pushboolean(L, ~mode & O_NONBLOCK);
	return 1;
    }
    int block = lua_toboolean(L, 2);
    if (block)
	mode &= ~O_NONBLOCK;
    else
	mode |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, mode) < 0)
	panic_error(L, "blocking fcntl set failure: %s");
    lua_pushboolean(L, 1);
    return 1;
}


LUALIB_API int luaopen_fdio(lua_State *L)
{
    static const luaL_Reg funcptrs[] = {
	FN_ENTRY(blocking),
	FN_ENTRY(status),
	{ NULL, NULL }
    };
    luaL_register(L, "fdio", funcptrs);
    
    return 1;
}
