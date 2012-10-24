CC=clang
CFLAGS=-Wall -Werror -std=gnu99 -ggdb
LDFLAGS=

OBJS_CLIENT=echoclient.o helper.o
OBJS_SERVER=echoserver.o helper.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

all: echoclient echoserver

echoclient: $(OBJS_CLIENT)
	$(CC) $(LDFLAGS) -o $@ $^

echoserver: $(OBJS_SERVER)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f echoclient
	rm -f echoserver
	rm -f $(OBJS_CLIENT) $(OBJS_SERVER)
