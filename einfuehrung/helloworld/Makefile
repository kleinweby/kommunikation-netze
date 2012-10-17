CC=clang
CFLAGS=-Wall -Werror -std=c99
LDFLAGS=

OBJ=helloworld.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

helloworld: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f helloworld
	rm -f $(OBJ)
