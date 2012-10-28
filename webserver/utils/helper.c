#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "helper.h"

char* stringFromSockaddrIn(struct sockaddr_in const* sockaddr) {
	char* buffer = malloc(100);
	
	if (!inet_ntop(sockaddr->sin_family, &sockaddr->sin_addr, buffer, 100))
		return NULL;
	
	size_t len = strlen(buffer);
	
	if (sockaddr->sin_family == AF_INET6) {
		memmove(buffer+1, buffer, strlen(buffer));
		buffer[0] = '[';
		buffer[len+1] = ']';
		len+=2;
    }
	
	snprintf(buffer+len, 99-len, ":%d", ntohs(sockaddr->sin_port));
	
	return buffer;
}

bool setBlocking(int socket, bool blocking) {
	int opts;
	
	opts = fcntl(socket, F_GETFL);
	if (opts < 0) {
		perror("fcntl");
		return false;
	}
	
	if (blocking)
		opts &= ~O_NONBLOCK;
	else
		opts |= O_NONBLOCK;
	
	if (fcntl(socket, F_SETFL, opts) < 0) {
		perror("fcntl");
		return false;
	}
	
	return true;
}
