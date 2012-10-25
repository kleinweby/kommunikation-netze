#include "server.h"
#include "helper.h"
#include "http.h"
#include "workerthread.h"

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
	
	
};

struct _pollCallback {
	WebServerPollBlock block;
};

struct _WebServer {
	Server* servers;
	uint16_t numberOfServers;
	
	bool keepRunning;
	
	struct pollfd* pollDescriptors;
	struct _pollCallback* pollCallbacks;
	uint32_t numOfPollDescriptors;
	uint32_t numOfPollDescriptorSlots;
};

static bool CreateServers(WebServer webServer, char* port);
static Server CreateServer(WebServer webServer, struct addrinfo *info);
static void ServerAcceptNewConnection(Server server);

WebServer WebServerCreate(char* port)
{
	WebServer webServer = malloc(sizeof(struct _WebServer));
	
	WorkerThreadsInitialize(10);
	
	webServer->numOfPollDescriptorSlots = 10;
	webServer->numOfPollDescriptors = 0;
	webServer->pollDescriptors = malloc(sizeof(struct pollfd) * webServer->numOfPollDescriptorSlots);
	webServer->pollCallbacks = malloc(sizeof(struct _pollCallback) * webServer->numOfPollDescriptorSlots);
	
	CreateServers(webServer, port);
	
	return webServer;
}

void WebServerRunloop(WebServer webServer)
{
	webServer->keepRunning = true;
	
	while (webServer->keepRunning) {
		int socksToHandle = poll(webServer->pollDescriptors, webServer->numOfPollDescriptors, 100000);
		
		// for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
		// 	printf("%d: fd %d, events %d, revents %d\n", i, webServer->pollDescriptors[i].fd, webServer->pollDescriptors[i].events, webServer->pollDescriptors[i].revents);
		// }
		
		if (socksToHandle < 0) {
			perror("poll");
			return;
		}
		else {
			for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
				if (webServer->pollDescriptors[i].revents > 0) {
					webServer->pollCallbacks[i].block(webServer->pollDescriptors[i].revents);
					webServer->pollDescriptors[i].revents = 0;
					socksToHandle--;
				}
			}
			
			if (socksToHandle != 0) {
				printf("Not handled %d\n", socksToHandle);
				for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
					if (webServer->pollDescriptors[i].revents > 0)
					printf("%d: fd %d, events %d, revents %d\n", i, webServer->pollDescriptors[i].fd, webServer->pollDescriptors[i].events, webServer->pollDescriptors[i].revents);
				}
			}
		}
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
	setBlocking(server->socket, false);
	
	WebServerRegisterPollForSocket(webServer, server->socket, POLLIN|POLLHUP, ^(int revents) {
		ServerAcceptNewConnection(server);
	});
	
	return server;
}

void WebServerRegisterPollForSocket(WebServer webServer, int socket, int events, WebServerPollBlock block)
{
	for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
		if (webServer->pollDescriptors[i].fd == socket) {
			webServer->pollDescriptors[i].events = events;
			if (webServer->pollCallbacks[i].block)
				Block_release(webServer->pollCallbacks[i].block);
			webServer->pollCallbacks[i].block = Block_copy(block);
			return;
		}
	}
	
	if (webServer->numOfPollDescriptors >= webServer->numOfPollDescriptorSlots) {
		webServer->numOfPollDescriptorSlots *= 2;
		webServer->pollDescriptors = realloc(webServer->pollDescriptors, sizeof(struct pollfd) * webServer->numOfPollDescriptorSlots);
		webServer->pollCallbacks = realloc(webServer->pollCallbacks, sizeof(struct _pollCallback) * webServer->numOfPollDescriptorSlots);
	}
	
	webServer->pollDescriptors[webServer->numOfPollDescriptors].fd = socket;
	webServer->pollDescriptors[webServer->numOfPollDescriptors].events = events;
	webServer->pollCallbacks[webServer->numOfPollDescriptors].block = Block_copy(block);
	
	webServer->numOfPollDescriptors++;
}

void WebServerUnregisterPollForSocket(WebServer webServer, int socket)
{
	for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
		if (webServer->pollDescriptors[i].fd == socket) {
			if (webServer->pollCallbacks[i].block)
				Block_release(webServer->pollCallbacks[i].block);
			
			memmove(&webServer->pollDescriptors[i], &webServer->pollDescriptors[i+1], sizeof(struct pollfd) * (webServer->numOfPollDescriptors - i));
			memmove(&webServer->pollCallbacks[i], &webServer->pollCallbacks[i+1], sizeof(struct _pollCallback) * (webServer->numOfPollDescriptors - i));
			webServer->numOfPollDescriptors--;
			return;
		}
	}
}

static void ServerAcceptNewConnection(Server server)
{
	HTTPConnection connection = HTTPConnectionCreate(server);
	#pragma unused(connection)
}

int ServerGetSocket(Server server)
{
	return server->socket;
}

WebServer ServerGetWebServer(Server server)
{
	return server->webServer;
}