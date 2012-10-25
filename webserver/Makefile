CC=clang
CFLAGS=-Wall -Werror -std=gnu99 -ggdb
LDFLAGS=

# Not the best but should work
HAS_NATIVE_BLOCKS=$(shell (uname -a | grep -q -i darwin) && echo 1 || echo 0)

OBJS=http.o test.o str_helper.o dictionary.o server.o helper.o workerthread.o

ifneq ($(HAS_NATIVE_BLOCKS), 1)
OBJS+= BlocksRuntime/libBlocksRuntime.a
CFLAGS+= -fblocks -IBlocksRuntime/BlocksRuntime
endif

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

all: webserver
	echo $(HAS_NATIVE_BLOCKS)

webserver: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

BlocksRuntime/libBlocksRuntime.a:
	cd BlocksRuntime && ./buildlib

clean:
	rm -f webserver
	rm -f BlocksRuntime/libBlocksRuntime.a
	rm -f $(OBJS)