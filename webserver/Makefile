CC=clang
CFLAGS=-Wall -Werror -std=gnu99 -ggdb
LDFLAGS=

OBJS=http.o test.o str_helper.o dictionary.o server.o helper.o workerthread.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

all: webserver

webserver: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f webserver
	rm -f $(OBJS_CLIENT) $(OBJS_SERVER)
