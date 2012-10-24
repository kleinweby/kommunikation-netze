#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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