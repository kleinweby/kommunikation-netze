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

#include "server.h"
#include "listener.h"
#include "utils/dictionary.h"
#include "client.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

DEFINE_CLASS(Server,
	bool keepRunning;

	Listener* listeners;
	uint16_t numberOfListeners;
	
	//
	// Dictionary that holds all known clients
	//
	Dictionary clients;
	
	//
	// 
	//
	Poll poll;
);

static void ServerDealloc(void* ptr);

static bool ServerCreateListeners(Server server, const char* port);

Server ServerCreate(const char* port)
{
	Server server = calloc(1, sizeof(struct _Server));
	
	if (server == NULL) {
		perror("calloc");
		return NULL;
	}
	
	ObjectInit(server, ServerDealloc);
	
	server->keepRunning = true;
	
	server->poll = PollCreate();
	server->clients = DictionaryCreate();
	
	if (!ServerCreateListeners(server, port)) {
		Release(server);
		return NULL;
	}
	
	return server;	
}

static void ServerDealloc(void* ptr)
{
	
	free(ptr);
}

void ServerMain(Server server)
{
	while (server->keepRunning)
		sleep(1000);
}

static bool ServerCreateListeners(Server server, const char* port)
{
	struct addrinfo *result;
	struct addrinfo hints;
	Listener* listeners = calloc(5, sizeof(Listener));
	uint16_t numberOfListeners = 0;
	uint16_t numberOfListenerSlots = 5;
	int error = 0;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	
	error = getaddrinfo(NULL, port, &hints, &result);
	if (error != 0) {
		free(listeners);
		printf("Error: %s", gai_strerror(error));
		return false;
	}
	
	for (struct addrinfo* serverInfo = result; serverInfo != NULL; serverInfo = serverInfo->ai_next) {
		Listener listener = CreateListener(server, serverInfo);
		
		if (listener) {			
			if (numberOfListeners >= numberOfListenerSlots) {
				numberOfListenerSlots *= 2;
				listeners = realloc(listeners, numberOfListenerSlots);
			}
			
			listeners[numberOfListeners] = listener;
			numberOfListeners++;
		}
	}
	
	freeaddrinfo(result);
	
	server->listeners = listeners;
	server->numberOfListeners = numberOfListeners;
	
	return true;
}

Poll ServerGetPoll(Server server)
{
	return server->poll;
}

Dictionary ServerGetClients(Server server)
{
	return server->clients;
}

void ServerSendChannelMessage(Server server, char* msg)
{
	DictionaryIterator iter;
	
	iter = DictionaryGetIterator(server->clients);
	
	do {
		Client c = DictionaryIteratorGetValue(iter);
		ClientWriteLine(c, msg);
	}
	while (DictionaryIteratorNext(iter));
}
