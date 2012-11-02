CC?=clang
CFLAGS=-Weverything -Wno-padded -Wno-missing-prototypes -Werror -std=gnu99 -ggdb -I. -pipe
LDFLAGS=-pipe

# Not the best but should work
IS_DARWIN=$(shell (uname -a | grep -q -i darwin) && echo 1 || echo 0)

SRC=http/http.c http/httpconnection.c http/httprequest.c http/httpresponse.c net/server.c net/poll.c utils/dictionary.c utils/dispatchqueue.c utils/helper.c utils/queue.c utils/retainable.c utils/str_helper.c utils/stack.c main.c
OBJS=$(SRC:.c=.o)

ifneq ($(IS_DARWIN), 1)
OBJS+= BlocksRuntime/libBlocksRuntime.a
CFLAGS+=-pthread -fblocks -IBlocksRuntime/BlocksRuntime
LDFLAGS+=-pthread
else
CFLAGS+=-DDARWIN
endif

# Makefile dependency to recompile when flags changed
%.o: %.c Makefile
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -MD -c -o $@ $<

%.E: %.c
	@echo "[PRE] $@"
	@$(CC) $(CFLAGS) -E -o $@ $<

all: webserver

webserver: $(OBJS)
	@echo "[LD] $@"
	@$(CC) $(LDFLAGS) -o $@ $^

BlocksRuntime/libBlocksRuntime.a:
	cd BlocksRuntime && ./buildlib

clean:
	rm -f webserver
	rm -f BlocksRuntime/libBlocksRuntime.a
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d)
	
-include $(OBJS:.o=.d)
