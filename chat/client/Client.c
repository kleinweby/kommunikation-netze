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

#include "Client.h"

#include "utils/helper.h"
#include "net/poll.h"

#include "gui-lib/chatgui.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <Block.h>

DEFINE_CLASS(Client,	
	int guiInFD;
	int guiOutFD;
	pid_t guiPID;
	
	int socket;
	
	Poll poll;
	bool keepRunning;
	
	char* sendBuffer[255];
	int sendHead;
	int sendTail;
);

static void ClientDealloc(void* ptr);
static bool ClientConnect(Client client, const char* host, const char* port);

Client ClientCreate(const char* host, const char* port)
{
	Client client = calloc(1, sizeof(struct _Client));
	
	if (client == NULL) {
		perror("calloc");
		return NULL;
	}
	
	ObjectInit(client, ClientDealloc);
	
	client->keepRunning = true;
	
	client->guiPID = gui_start(&client->guiInFD, &client->guiOutFD);
	
	if (client->guiPID == -1) {
		perror("gui_start");
		Release(client);
		return NULL;
	}
	
	client->poll = PollCreate();
	
	if (!client->poll) {
		Release(client);
		return NULL;
	}
	
	if (!ClientConnect(client, host, port)) {
		ClientPrintf(client, "Could not connect to any server\n");
		Release(client);
		return NULL;
	}
	
	// PollRegister(client->poll, client->socket, POLLIN, kPollRepeatFlag, NULL, ^(short revents) {
	// 	if ((revents & POLLHUP) > 0) {
	// 		ClientPrintf(client, "Error in the connection with the server\n");
	// 		return;
	// 	}
	// 	
	// 	if ((revents & POLLIN) > 0) {
	// 		// Read
	// 		char buffer[255];
	// 		ssize_t len;
	// 		
	// 		len = recv(client->socket, buffer, 255, 0);
	// 		if (len < 0) {
	// 			ClientPrintf(client, "Error reading");
	// 			return;
	// 		}
	// 		write(client->guiOutFD, buffer, (size_t)len);
	// 	}
	// });
	// 
	// PollRegister(client->poll, client->guiInFD, POLLIN, kPollRepeatFlag, NULL, ^(short revents) {
	// 	if ((revents & POLLIN) > 0) {
	// 		char *buffer = calloc(1, 255);
	// 		ssize_t len;
	// 		
	// 		len = read(client->guiInFD, buffer, 254);
	// 		
	// 		if (len < 0) {
	// 			perror("read");
	// 			return;
	// 		}
	// 					
	// 		{
	// 			__block size_t sentBytes = 0;
	// 			
	// 			__block void (^block)() = Block_copy(^{
	// 				ssize_t s;
	// 				
	// 				s = send(client->socket, buffer+sentBytes, (size_t)len-sentBytes, 0);
	// 				
	// 				if (s < 0) {
	// 					perror("send");
	// 					return;
	// 				}
	// 				
	// 				sentBytes += (size_t)s;
	// 				
	// 				if (sentBytes < (size_t)len) {
	// 					PollRegister(client->poll, client->socket, POLLOUT, 0, NULL, ^(short revents2) {
	// 						if ((revents2 & POLLOUT) > 0) {
	// 							block();
	// 						}
	// 					});
	// 				}
	// 				else {
	// 					free(buffer);
	// 				}
	// 			});
	// 			
	// 			block();
	// 			Block_release(block);
	// 		}
	// 	}
	// });
	
	return client;
}

static void ClientDealloc(void* ptr)
{
	
	free(ptr);
}

void ClientMain(Client client)
{
	while (client->keepRunning) sleep(100);
}

void ClientPrintf(Client client, char* format, ...)
{
	char* str;
	
	va_list ap;
	va_start(ap, format);
	vasprintf(&str, format, ap);
	va_end(ap);
	
	// Not interested in status
	write(client->guiOutFD, str, strlen(str));
}

static bool ClientConnect(Client client, const char* host, const char* port)
{	
	int sock = -1;
    int error;
	struct addrinfo *result;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
    error = getaddrinfo(host, port, &hints, &result);
	if (error != 0) {
		ClientPrintf(client, "Error: %s\n", gai_strerror(error));
		printf("Error: %s\n", gai_strerror(error));
		return -1;
	}
	
	// Cylce through the results an try to connect
	for (struct addrinfo* server = result; server != NULL; server = server->ai_next) {
		sock = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	
		if (sock < 0) {
			continue;
		}
		
		{
			char* desc = stringFromSockaddrIn(server->ai_addr, server->ai_addrlen);
			
			if (desc) {
				ClientPrintf(client, "Try connecting to %s...\n", desc);
				free(desc);
			}
		}
		
		if (connect(sock, server->ai_addr, server->ai_addrlen) < 0) {
			close(sock);
			sock = -1;
		} 
		else
			break;
	}
	
	freeaddrinfo(result);
	
	client->socket = sock;
	
	return sock > 0;
}
