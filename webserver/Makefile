CC=clang
CFLAGS=-Wall -Werror -std=gnu99 -ggdb
LDFLAGS=

# Not the best but should work
IS_DARWIN=$(shell (uname -a | grep -q -i darwin) && echo 1 || echo 0)

OBJS=http.o test.o str_helper.o dictionary.o server.o helper.o queue.o retainable.o dispatchqueue.o poll.o

ifneq ($(IS_DARWIN), 1)
OBJS+= BlocksRuntime/libBlocksRuntime.a
CFLAGS+=-pthread -fblocks -IBlocksRuntime/BlocksRuntime
LDFLAGS+=-pthread
endif

%.o: %.c
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -MD -c -o $@ $^

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
