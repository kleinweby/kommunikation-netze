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

#include "listener.h"
#include "utils/helper.h"
#include "client.h"

#include <stdlib.h>
#include <stdio.h>

DEFINE_CLASS(Listener,
	int socket;

	//
	// Server reference
	//
	Server server;

	//
	// Saved informations about the server socket
	//
	struct sockaddr_in6 info;
	socklen_t infoLength;
	
	PollDescriptor pollDescriptor;
);

static void ListenerDealloc(void* ptr);

static void ListenerHandleNewClient(Listener listener, int socket, struct sockaddr_in6 info);

Listener CreateListener(Server server, struct addrinfo *addrInfo)
{
	Listener listener = calloc(1, sizeof(struct _Listener));
	
	if (listener == NULL) {
		perror("calloc");
		return NULL;
	}
	
	ObjectInit(listener, ListenerDealloc);
	
	listener->server = server;
	
	listener->socket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
		
	if (listener->socket < 0) {
		Release(listener);
		return NULL;
	}
	
	// make the socket reusable
	static const int kOn = 1;
	if (setsockopt(listener->socket, SOL_SOCKET, SO_REUSEADDR, &kOn, sizeof(kOn)) < 0) {
		perror("setsockopt");
		Release(listener);
		return NULL;
	}
	
	// Bind to the socket
	if (bind(listener->socket, addrInfo->ai_addr, addrInfo->ai_addrlen) < 0) {
		Release(listener);
		return NULL;
	}

	listener->infoLength = sizeof(listener->info);
	if (getsockname(listener->socket, (struct sockaddr*)&listener->info, &listener->infoLength) < 0) {
		Release(listener);
		return NULL;
	}
	
	{
		char *s = stringFromSockaddrIn(&listener->info, listener->infoLength);
		printf("Listen on %s...\n", s);
		free(s);
	}
	listen(listener->socket, 10);
	
	listener->pollDescriptor = PollRegister(ServerGetPoll(server), listener->socket, POLLIN, NULL, ^(short revents) {
		if ((revents & POLLHUP) > 0) {
			printf("Error pollling server socket?!\n");
			return;
		}
		// New client
		else if ((revents & POLLIN) > 0) {
			struct sockaddr_in6 info;
			socklen_t infoSize = sizeof(info);
			int socket = accept(listener->socket, (struct sockaddr*)&info, &infoSize);
			
			ListenerHandleNewClient(listener, socket, info);
		}
	});
	
	return listener;	
}

static void ListenerDealloc(void* ptr)
{
	free(ptr);
}

static void ListenerHandleNewClient(Listener listener, int socket, struct sockaddr_in6 info)
{
	Client c = ClientCreate(listener, socket, info);
	Release(c);
}

Poll ListenerGetPoll(Listener listener)
{
	return ServerGetPoll(listener->server);
}

Server ListenerGetServer(Listener listener)
{
	return listener->server;
}
