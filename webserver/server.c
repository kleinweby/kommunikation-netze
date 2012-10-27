#include "server.h"
#include "helper.h"
#include "http.h"
#include "dispatchqueue.h"
#include "queue.h"

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

struct _Server {
	WebServer webServer;
	
	//
	// The socket to accept incomming connections
	//
	int socket;
	
	//
	// Saved informations about the server socket
	//
	struct sockaddr_in info;
	socklen_t infoLength;
	
	pthread_t acceptThread;
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
	
	webServer->poll = PollCreate();
	
	CreateServers(webServer, port);
	
	if (webServer->numberOfServers == 0) {
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
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	
	if (getaddrinfo(NULL, port, &hints, &result) < 0) {
		perror("getaddrinfo");
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
	
	webServer->servers = servers;
	webServer->numberOfServers = numberOfServers;
	
	return true;
}

static Server CreateServer(WebServer webServer, struct addrinfo *info)
{
	Server server = malloc(sizeof(struct _Server));
	
	server->webServer = webServer;
	server->socket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		
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
	
	if (bind(server->socket, info->ai_addr, info->ai_addrlen) < 0) {
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
	
	PollRegister(server->webServer->poll, server->socket, POLLIN, kPollRepeatFlag, NULL, ^(int revents) {
		struct sockaddr info;
		socklen_t infoSize = sizeof(info);
		int socket = accept(server->socket, &info, &infoSize);
		
		if (socket) {
			HTTPConnection connection = HTTPConnectionCreate(server, socket, info);
			#pragma unused(connection)
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
	return server->webServer->poll
		;}

WebServer ServerGetWebServer(Server server)
{
	return server->webServer;
}