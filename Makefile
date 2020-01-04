CFLAGS+=-fPIC -I /usr/local/include/luajit-2.0/
CFLAGS+=-D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE
CFLAGS+=-I../../c_library_v1/common -I../common
CFLAGS+=-O3 -march=native
#CFLAGS+=-mfloat-abi=hard
CFLAGS+=-Wall -Wno-parentheses -std=c99 -DUSE_TCP_NODELAY
LDFLAGS+=-lluajit-5.1 -lm '-Wl,-R$$ORIGIN'

CFLAGS+=-fomit-frame-pointer

#CFLAGS+=-g -fno-omit-frame-pointer
#LDFLAGS+= -fsanitize=address

.phony: all clean

all: djicrc.so fdio.so

%.so: %.o
	gcc -shared $(LDFLAGS) -o $@ $^

clean:
	find \( -name \*.o -delete -o -name \*.so \) -delete
