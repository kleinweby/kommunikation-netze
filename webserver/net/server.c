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
#include "utils/helper.h"
#include "http/http.h"
#include "http/httpconnection.h"
#include "utils/dispatchqueue.h"
#include "utils/queue.h"

#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <Block.h>
#include <assert.h>
#include <pthread.h>
#ifdef LINUX
#include <signal.h>
#endif

struct _Server {
	WebServer webServer;
	
	//
	// The socket to accept incomming connections
	//
	int socket;
	
	//
	// Saved informations about the server socket
	//
	struct sockaddr_in6 info;
	socklen_t infoLength;
};

struct _WebServer {
	Server* servers;
	uint16_t numberOfServers;
	
	bool keepRunning;

	DispatchQueue inputQueue;
	DispatchQueue processingQueue;
	DispatchQueue outputQueue;
	
	Queue pollUpdateQueue;
	Poll poll;
};

static bool CreateServers(WebServer webServer, char* port);
static Server CreateServer(WebServer webServer, struct addrinfo *info);

WebServer WebServerCreate(char* port)
{
	WebServer webServer = malloc(sizeof(struct _WebServer));
		
	webServer->inputQueue = DispatchQueueCreate(0);
	webServer->outputQueue = DispatchQueueCreate(0);
	webServer->processingQueue = DispatchQueueCreate(0);
	webServer->numberOfServers = 0;
	webServer->poll = PollCreate();
	
	signal(SIGPIPE, SIG_IGN);
	CreateServers(webServer, port);
	
	if (webServer->numberOfServers == 0) {
		Release(webServer);
		printf("Could not listen on any socket.\n");
		return NULL;
	}
	
	return webServer;
}

void WebServerRunloop(WebServer webServer)
{
	webServer->keepRunning = true;
		
	while (webServer->keepRunning) {
		sleep(10000000);
	}
}

static bool CreateServers(WebServer webServer, char* port)
{
	struct addrinfo *result;
	struct addrinfo hints;
	Server* servers = malloc(5);
	uint16_t numberOfServers = 0;
	uint16_t numberOfServerSlots = 5;
	int error = 0;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	
	error = getaddrinfo(NULL, port, &hints, &result);
	if (error != 0) {
		free(servers);
		printf("Error: %s", gai_strerror(error));
		return false;
	}
	
	for (struct addrinfo* serverInfo = result; serverInfo != NULL; serverInfo = serverInfo->ai_next) {
		Server server = CreateServer(webServer, serverInfo);
		
		if (server) {			
			if (numberOfServers >= numberOfServerSlots) {
				numberOfServerSlots *= 2;
				servers = realloc(servers, numberOfServerSlots);
			}
			
			servers[numberOfServers] = server;
			numberOfServers++;
		}
	}
	
	freeaddrinfo(result);
	
	webServer->servers = servers;
	webServer->numberOfServers = numberOfServers;
	
	return true;
}

static Server CreateServer(WebServer webServer, struct addrinfo *serverInfo)
{
	Server server = malloc(sizeof(struct _Server));
	
	server->webServer = webServer;
	server->socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
		
	if (server->socket < 0) {
		free(server);
		return NULL;
	}
		
	static const int kOn = 1;
	if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &kOn, sizeof(kOn)) < 0) {
		perror("setsockopt");
		close(server->socket);
		free(server);
		return NULL;
	}
	
	if (bind(server->socket, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) {
		close(server->socket);
		free(server);
		return NULL;
	}

	server->infoLength = sizeof(server->info);
	if (getsockname(server->socket, (struct sockaddr*)&server->info, &server->infoLength) < 0) {
		close(server->socket);
		free(server);
		return NULL;
	}
	
	printf("Listen on %s...\n", stringFromSockaddrIn(&server->info));
	listen(server->socket, 300);
		
	PollRegister(server->webServer->poll, server->socket, POLLIN, kPollRepeatFlag, NULL, ^(short revents) {
#pragma unused(revents)
		struct sockaddr_in6 info;
		socklen_t infoSize = sizeof(info);
		int socket = accept(server->socket, (struct sockaddr*)&info, &infoSize);
		setTCPNoPush(socket, true);
		if (socket) {
			HTTPConnection connection = HTTPConnectionCreate(server, socket, info);
			Release(connection);
		}
	});
	
	return server;
}

int ServerGetSocket(Server server)
{
	return server->socket;
}

DispatchQueue ServerGetInputDispatchQueue(Server server)
{
	return server->webServer->inputQueue;
}

DispatchQueue ServerGetOutputDispatchQueue(Server server)
{
	return server->webServer->outputQueue;
}

DispatchQueue ServerGetProcessingDispatchQueue(Server server)
{
	return server->webServer->processingQueue;
}

Poll ServerGetPoll(Server server)
{
	return server->webServer->poll;
}

WebServer ServerGetWebServer(Server server)
{
	return server->webServer;
}
