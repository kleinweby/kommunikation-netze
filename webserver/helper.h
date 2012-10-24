#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

char* stringFromSockaddrIn(struct sockaddr_in const* sockaddr);
bool setBlocking(int socket, bool blocking);