#include <termios.h>
#include "lua_head.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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


LUAFN(open)
{
    const char *ttyname = luaL_checkstring(L, 1);
    int speed = luaL_checkinteger(L, 2);
    tcflag_t hdwflow = lua_toboolean(L, 3) ? CRTSCTS : 0;
    int fd = -1;
    struct termios ttysettings;
    
    if ((fd = open(ttyname, O_RDWR | O_NOCTTY)) < 0)
	panic_error(L, "serial open failure: %s");

    if (tcgetattr(fd, &ttysettings) != 0)
	panic_error(L, "serial open (tcgetattr) failure: %s");

    cfmakeraw(&ttysettings);
    ttysettings.c_cflag = ttysettings.c_cflag & ~CRTSCTS | hdwflow;
    ttysettings.c_iflag |= IGNBRK;
    ttysettings.c_lflag &= ~(ECHOE | ECHOK | ECHOCTL | ECHOKE);
    ttysettings.c_oflag &= ~ONLCR;
    cfsetispeed(&ttysettings, speed);
    cfsetospeed(&ttysettings, speed);

    if (tcsetattr(fd, TCSANOW, &ttysettings) != 0)
	panic_error(L, "serial open (tcsetattr) failure: %s");

    lua_pushinteger(L, fd);
    return 1;
}

LUAFN(change_speed)
{
    int fd = luaL_checkinteger(L, 1);
    int speed = luaL_checkinteger(L, 2);
    struct termios ttysettings;
    if (tcgetattr(fd, &ttysettings) != 0)
	panic_error(L, "tcgetattr failure: %s");
    cfsetispeed(&ttysettings, speed);
    cfsetospeed(&ttysettings, speed);
    if (tcsetattr(fd, TCSANOW, &ttysettings) != 0)
	panic_error(L, "tcsetattr failure: %s");
    lua_pushboolean(L, 1);
    return 1;
}

LUAFN(close)
{
    int fd = luaL_checkinteger(L, 1);
    
    tcflush(fd, TCIOFLUSH);
    close(fd);
    return 0;
}

typedef struct { const char *name; int value; } intconst;

#define BAUD(NAME) { #NAME, NAME }

LUALIB_API int luaopen_termios(lua_State *L)
{
    static intconst bauds[] = {
	BAUD(B0),
	BAUD(B50),
	BAUD(B75),
	BAUD(B110),
	BAUD(B134),
	BAUD(B150),
	BAUD(B200),
	BAUD(B300),
	BAUD(B600),
	BAUD(B1200),
	BAUD(B1800),
	BAUD(B2400),
	BAUD(B4800),
	BAUD(B9600),
	BAUD(B19200),
	BAUD(B38400),
	BAUD(B57600),
	BAUD(B115200),
	BAUD(B230400),
	// High speed serial for PixHawk.
	BAUD(B921600),
	{NULL, 0}
    };
    
    static const luaL_Reg funcptrs[] = {
	FN_ENTRY(open),
	FN_ENTRY(change_speed),
	FN_ENTRY(close),
	{ NULL, NULL }
    };
    luaL_register(L, "termios", funcptrs);

    lua_pushstring(L, "baudrate");
    lua_newtable(L);
    for (int i = 0; bauds[i].name; i++) {
	lua_pushstring(L, bauds[i].name);
	lua_pushinteger(L, bauds[i].value);
	lua_rawset(L, -3);
    }
    lua_rawset(L, -3);

    
    return 1;
}
