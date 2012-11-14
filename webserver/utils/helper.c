// Copyright (c) 2012, Christian Speich <christian@spei.ch>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include "helper.h"

char* stringFromSockaddrIn4(struct sockaddr_in const* sockaddr) {
	char* buffer = malloc(100);
		
	if (!inet_ntop(sockaddr->sin_family, &sockaddr->sin_addr, buffer, 100)) {
		free(buffer);
		return NULL;
	}
	
	size_t len = strlen(buffer);
	
	snprintf(buffer+len, 99-len, ":%d", ntohs(sockaddr->sin_port));
	
	return buffer;
}

char* stringFromSockaddrIn6(struct sockaddr_in6 const* sockaddr) {
	char* buffer = malloc(100);
	
	buffer[0] = '[';
	
	if (!inet_ntop(sockaddr->sin6_family, &sockaddr->sin6_addr, buffer + 1, 99))
		return NULL;
	
	size_t len = strlen(buffer);

	buffer[len] = ']';
	len++;
	
	snprintf(buffer+len, 99-len, ":%d", ntohs(sockaddr->sin6_port));
	
	return buffer;
}

char* stringFromSockaddrIn(struct sockaddr_in6 const* sockaddr) {
	if (sockaddr->sin6_family == AF_INET6) {
		return stringFromSockaddrIn6(sockaddr);
    }
	else {
		return stringFromSockaddrIn4((struct sockaddr_in const*)sockaddr);
	}
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

bool setTCPNoPush(int socket, bool noPush)
{
	int state = noPush;
	
#ifdef DARWIN
	return setsockopt(socket, IPPROTO_TCP, TCP_NOPUSH, &state, sizeof(state)) == 0;
#else
	return setsockopt(socket, IPPROTO_TCP, TCP_CORK, &state, sizeof(state)) == 0;
#endif
}
