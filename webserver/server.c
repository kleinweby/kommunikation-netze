#include "server.h"
#include "helper.h"
#include "http.h"
#include "workerthread.h"
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

struct _pollCallback {
	WebServerPollBlock block;
};

struct _PollUpdate{
	struct pollfd descriptor;
	//
	// When block is null, means delete the poll observer
	//
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
	int pollNeedsUpDateFd[2];
	
	Queue pollUpdateQueue;
};

static bool CreateServers(WebServer webServer, char* port);
static Server CreateServer(WebServer webServer, struct addrinfo *info);
static void *ServerAcceptLoop(void* ptr);
static void WebServerUpdatePollDescriptors(WebServer webServer);

WebServer WebServerCreate(char* port)
{
	WebServer webServer = malloc(sizeof(struct _WebServer));
	
	WorkerThreadsInitialize(20);
	
	webServer->numOfPollDescriptorSlots = 10;
	webServer->numOfPollDescriptors = 0;
	webServer->pollDescriptors = malloc(sizeof(struct pollfd) * webServer->numOfPollDescriptorSlots);
	webServer->pollCallbacks = malloc(sizeof(struct _pollCallback) * webServer->numOfPollDescriptorSlots);
	webServer->pollUpdateQueue = QueueCreate();
	pipe(webServer->pollNeedsUpDateFd);
	setBlocking(webServer->pollNeedsUpDateFd[0], false);
	setBlocking(webServer->pollNeedsUpDateFd[0], true);
	
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
	
	WebServerRegisterPollForSocket(webServer, webServer->pollNeedsUpDateFd[0], POLLIN, ^(int revents){});
	
	while (webServer->keepRunning) {
		WebServerUpdatePollDescriptors(webServer);
		
		int socksToHandle = poll(webServer->pollDescriptors, webServer->numOfPollDescriptors, 1000);
		
		WebServerUpdatePollDescriptors(webServer);
		
		if (socksToHandle < 0) {
			perror("poll");
			return;
		}
		else if (socksToHandle > 0) {
			for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
				if (webServer->pollDescriptors[i].revents > 0) {
					if (webServer->pollDescriptors[i].fd == webServer->pollNeedsUpDateFd[0]) {
						char buffer[255];
						// Just read it away
						read(webServer->pollNeedsUpDateFd[0], &buffer, 255);
						socksToHandle--;
					}
					else {
					// Enqueue an unregister now, to let it be overriden by the block
					WebServerUnregisterPollForSocket(webServer, webServer->pollDescriptors[i].fd);
					
					int revents = webServer->pollDescriptors[i].revents;
					WebServerPollBlock block = Block_copy(webServer->pollCallbacks[i].block);
					
					WorkerThreadsEnqueue(^{
						block(revents);
						Block_release(block);
					});
					
					webServer->pollDescriptors[i].revents = 0;

					socksToHandle--;
					}
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
	setBlocking(server->socket, false);
	
	if (pthread_create(&server->acceptThread, NULL, ServerAcceptLoop, server)) {
		perror("pthread_create");
	}
	
	return server;
}

void WebServerRegisterPollForSocket(WebServer webServer, int socket, int events, WebServerPollBlock block)
{	
	struct _PollUpdate* update = malloc(sizeof(struct _PollUpdate));
	
	memset(update, 0, sizeof(struct _PollUpdate));
	
	update->descriptor.fd = socket;
	update->descriptor.events = events;
	update->block = Block_copy(block);
	
	QueueEnqueue(webServer->pollUpdateQueue, update);
	int i = 1;
	write(webServer->pollNeedsUpDateFd[1], &i, 1);
}

void WebServerUnregisterPollForSocket(WebServer webServer, int socket)
{
	struct _PollUpdate* update = malloc(sizeof(struct _PollUpdate));
	
	memset(update, 0, sizeof(struct _PollUpdate));
	
	update->descriptor.fd = socket;
	
	QueueEnqueue(webServer->pollUpdateQueue, update);
	int i = 1;
	write(webServer->pollNeedsUpDateFd[1], &i, 1);
}

void WebServerUpdatePollDescriptors(WebServer webServer) {
	struct _PollUpdate* update;
	
	while ((update = QueueDequeue(webServer->pollUpdateQueue)) != NULL) {
		// Add
		if (update->block) {
			bool found = false;
			for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
				if (webServer->pollDescriptors[i].fd == update->descriptor.fd) {
					webServer->pollDescriptors[i].events = update->descriptor.events;
					if (webServer->pollCallbacks[i].block)
						Block_release(webServer->pollCallbacks[i].block);
					webServer->pollCallbacks[i].block = update->block;
					found = true;
					break;
				}
			}
	
			if (!found) {
				if (webServer->numOfPollDescriptors >= webServer->numOfPollDescriptorSlots) {
					webServer->numOfPollDescriptorSlots *= 2;
					webServer->pollDescriptors = realloc(webServer->pollDescriptors, sizeof(struct pollfd) * webServer->numOfPollDescriptorSlots);
					webServer->pollCallbacks = realloc(webServer->pollCallbacks, sizeof(struct _pollCallback) * webServer->numOfPollDescriptorSlots);
				}
	
				webServer->pollDescriptors[webServer->numOfPollDescriptors].fd = update->descriptor.fd;
				webServer->pollDescriptors[webServer->numOfPollDescriptors].events = update->descriptor.events;
				webServer->pollCallbacks[webServer->numOfPollDescriptors].block = update->block;
	
				webServer->numOfPollDescriptors++;
			}
		}
		// Delete
		else {
			for(int i = 0; i < webServer->numOfPollDescriptors; i++) {
				if (webServer->pollDescriptors[i].fd == update->descriptor.fd) {
					if (webServer->pollCallbacks[i].block)
						Block_release(webServer->pollCallbacks[i].block);
			
					if (i != webServer->numOfPollDescriptors - 1) {
						memcpy(&webServer->pollDescriptors[i], &webServer->pollDescriptors[webServer->numOfPollDescriptors - 1], sizeof(struct pollfd));
						memcpy(&webServer->pollCallbacks[i], &webServer->pollCallbacks[webServer->numOfPollDescriptors - 1], sizeof(struct _pollCallback));
					}
						
					webServer->numOfPollDescriptors--;
					break;
				}
			}
		}
		free(update);
	}
}

static void* ServerAcceptLoop(void* ptr)
{	
	Server server = ptr;
	
	setBlocking(server->socket, true);
	while (true) {
		HTTPConnection connection = HTTPConnectionCreate(server);
		#pragma unused(connection)
	}
}

int ServerGetSocket(Server server)
{
	return server->socket;
}

WebServer ServerGetWebServer(Server server)
{
	return server->webServer;
}