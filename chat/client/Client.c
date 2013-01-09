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
#include <signal.h>


DEFINE_CLASS(Client,	
	int guiInFD;
	int guiOutFD;
	pid_t guiPID;
	
	int socket;
	
	Poll poll;
	bool keepRunning;
	
	PollDescriptor socketPollDescriptor;
	PollDescriptor guiOutPollDescriptor;
	PollDescriptor guiInPollDescriptor;
	
	// Recving
	char* inBuffer;
	uint32_t inBufferLength;
	uint32_t inBufferFilled;
	
	// Sending
	char* outBuffer;
	uint32_t outBufferLength;
	uint32_t outBufferFilled;
);

static void ClientDealloc(void* ptr);
static bool ClientConnect(Client client, const char* host, const char* port);
static void ClientReceive(Client client);
static void ClientSend(Client client);
static void ClientGUIRead(Client client);
static void ClientGUIWrite(Client client);

Client ClientCreate(const char* host, const char* port)
{
printf("Client\n");
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
                printf("poll");
		Release(client);
		return NULL;
	}
	
	if (!ClientConnect(client, host, port)) {
		ClientPrintf(client, "Could not connect to any server\n");
		Release(client);
		return NULL;
	}
	
	client->socketPollDescriptor = PollRegister(client->poll, client->socket, POLLIN, NULL, ^(short revents) {
		if ((revents & POLLHUP) > 0) {
			client->keepRunning = false;
		}
		else if ((revents & POLLIN) > 0) {
			ClientReceive(client);
		}
		else if ((revents & POLLOUT) > 0) {
			ClientSend(client);
		}
	});
	
	client->guiInPollDescriptor = PollRegister(client->poll, client->guiInFD, POLLIN, NULL, ^(short revents) {
		if ((revents & POLLHUP) > 0) {
			client->keepRunning = false;
		}
		else if ((revents & POLLIN) > 0) {
			ClientGUIRead(client);
		}
	});
	
	client->guiOutPollDescriptor = PollRegister(client->poll, client->guiOutFD, 0, NULL, ^(short revents) {
		if ((revents & POLLHUP) > 0) {
			client->keepRunning = false;
		}
		else if ((revents & POLLOUT) > 0) {
			ClientGUIWrite(client);
		}
	});
	
	return client;
}

static void ClientDealloc(void* ptr)
{
	Client client = ptr;
	kill(client->guiPID, SIGTERM);
	close(client->guiInFD);
	close(client->guiOutFD);
	close(client->socket);
	free(ptr);
}

void ClientMain(Client client)
{
	while (client->keepRunning) sleep(1);
	
	printf("Quitting...");
	kill(client->guiPID, SIGTERM);
}

void ClientPrintf(Client client, char* format, ...)
{
	char* str;
	
	va_list ap;
	va_start(ap, format);
	str = malloc((size_t)vsnprintf(NULL, 0, format, ap) + 1);
	va_end(ap);
	if (str == NULL) abort();
	va_start(ap, format);
	vsprintf(str, format, ap);
	va_end(ap);
	
	// Not interested in status
	write(client->guiOutFD, str, strlen(str));
	free(str);
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
			perror("socket");
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
			perror("connect");
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

static void ClientReceive(Client client)
{
	// Make room in the buffer, if non avaiable
	if (client->inBufferLength - client->inBufferFilled < 1 /* something */) {
		client->inBufferLength *= 2;
		if (client->inBufferLength < 1024)
			client->inBufferLength = 1024;
		client->inBuffer = realloc(client->inBuffer, client->inBufferLength);
		if (client->inBuffer == NULL) {
			perror("realloc");
			// TODO
			return;
		}
		
		// Initialize the new buffer to zero
		memset(client->inBuffer+client->inBufferFilled, 0, client->inBufferLength - client->inBufferFilled);
	}
	
	ssize_t len = recv(client->socket, client->inBuffer+client->inBufferFilled, client->inBufferLength - client->inBufferFilled, 0);
	
	if (len < 0) {
		perror("recv");
		abort();
	}
	else if (len == 0) {
		client->keepRunning = false;
	}
	else {
		client->inBufferFilled += len;
		
		// Inform that we want to send, if we're not already sending
		PollDescriptorAddEvent(client->guiOutPollDescriptor, POLLOUT);
	}
}

static void ClientSend(Client client)
{
	ssize_t len;
	
	len = send(client->socket, client->outBuffer, client->outBufferFilled, 0);
	
	if (len < 0) {
		perror("send");
		abort();
	}
	else if (len == 0) {
		client->keepRunning = false;
	}
	else {
		// Everything was send, so discard buffer
		if (client->outBufferFilled == len) {
			client->outBufferFilled = client->outBufferLength = 0;
			free(client->outBuffer);
			client->outBuffer = NULL;
			
			// Don't want to send anymore
			PollDescriptorRemoveEvent(client->socketPollDescriptor, POLLOUT);
		}
		else {
			memmove(client->outBuffer, client->outBuffer+len, (size_t)(client->outBufferFilled - len));
			// Don't shrink the buffer, only deallocated as seen above.
		}
	}
}

static void ClientGUIRead(Client client)
{
	// Make room in the buffer, if non avaiable
	if (client->outBufferLength - client->outBufferFilled < 1 /* something */) {
		client->outBufferLength *= 2;
		if (client->outBufferLength < 1024)
			client->outBufferLength = 1024;
		client->outBuffer = realloc(client->outBuffer, client->outBufferLength);
		if (client->outBuffer == NULL) {
			perror("realloc");
			// TODO
			return;
		}
		
		// Initialize the new buffer to zero
		memset(client->outBuffer+client->outBufferFilled, 0, client->outBufferLength - client->outBufferFilled);
	}
	
	ssize_t len = read(client->guiInFD, client->outBuffer+client->outBufferFilled, client->outBufferLength - client->outBufferFilled);
	
	if (len < 0) {
		perror("recv");
		abort();
	}
	else if (len == 0) {
		client->keepRunning = false;
	}
	else {
		client->outBufferFilled += len;
		
		// Inform that we want to send, if we're not already sending
		PollDescriptorAddEvent(client->socketPollDescriptor, POLLOUT);
	}
}

static void ClientGUIWrite(Client client)
{
	ssize_t len;
	
	len = write(client->guiOutFD, client->inBuffer, client->inBufferFilled);
	
	if (len < 0) {
		perror("send");
		abort();
	}
	else if (len == 0) {
		client->keepRunning = false;
	}
	else {
		// Everything was send, so discard buffer
		if (client->inBufferFilled == len) {
			client->inBufferFilled = client->inBufferLength = 0;
			free(client->inBuffer);
			client->inBuffer = NULL;
			
			// Don't want to send anymore
			PollDescriptorRemoveEvent(client->guiOutPollDescriptor, POLLOUT);
		}
		else {
			memmove(client->inBuffer, client->inBuffer+len, (size_t)(client->inBufferFilled - len));
			// Don't shrink the buffer, only deallocated as seen above.
		}
	}
}
