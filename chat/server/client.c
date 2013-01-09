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

#include "client.h"

#include "utils/helper.h"
#include "utils/str_helper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

DEFINE_CLASS(Client, 
	Listener listener;

	int socket;
	
	struct sockaddr_in6 addrInfo;
	char* addrInfoLine;
	
	PollDescriptor pollDescriptor;
	
	char* nickname;
	
	// Reading
	char* inBuffer;
	uint32_t inBufferLength;
	uint32_t inBufferFilled;
	
	// Writing
	char* outBuffer;
	uint32_t outBufferLength;
	uint32_t outBufferFilled;
);

static void ClientDealloc(void* ptr);

// This makes a private copy of nickname if needed
static bool ClientSetNickname(Client client, char* nickname);
static void ClientDoRead(Client client);
static void ClientDoWrite(Client client);
static void ClientHandleLine(Client client, char* line);

static void ClientHandleNickCommand(Client client, char* arg);

struct CommandDef {
	char* name;
	void (*handler)(Client client, char* arg);
};

struct CommandDef CommandDefs[] = {
	{"nick", ClientHandleNickCommand},
	{NULL, NULL}
};

Client ClientCreate(Listener listener, int socket, struct sockaddr_in6 addrInfo)
{
	Client client = malloc(sizeof(struct _Client));
	
	if (client == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(client, 0, sizeof(struct _Client));
	
	ObjectInit(client, ClientDealloc);
	client->listener = listener;
	client->socket = socket;
	client->addrInfo = addrInfo;
	client->addrInfoLine = stringFromSockaddrIn(&client->addrInfo, sizeof(client->addrInfo));
	
	printf("New client:%p from %s...\n", client, client->addrInfoLine);
	
	client->pollDescriptor = PollRegister(ListenerGetPoll(listener), client->socket, POLLIN, NULL, ^(short revents) {
		if ((revents & POLLHUP) > 0) {
			ClientDisconnect(client, "Client disconnected");
		}
		else if ((revents & POLLIN) > 0) {
			ClientDoRead(client);
		}
		else if ((revents & POLLOUT) > 0) {
			ClientDoWrite(client);
		}
	});
	
	{
		uint32_t i;
		for (i = 1; i < UINT32_MAX; i++) {
			char* nick;
		
			if (asprintf(&nick, "Derp%d", i) >= 0) {
				if (ClientSetNickname(client, nick)) {
					free(nick);
					break;
				}
				free(nick);
			}
		}
		
		if (i == UINT32_MAX) {
			// TODO fail
		}
	}
	
	{
		char* msg;
		
		if (asprintf(&msg, "%s has entered...", client->nickname) >= 0) {
			ServerSendChannelMessage(ListenerGetServer(client->listener), msg);
			free(msg);
		}
	}
	
	return client;
}

static void ClientDealloc(void* ptr)
{
	free(ptr);
}

void ClientDisconnect(Client client, char* reason)
{
	printf("Disconnected client %p:%s: %s\n", client, client->addrInfoLine, reason);
	DictionaryRemove(ServerGetClients(ListenerGetServer(client->listener)), client->nickname);
	{
		char* msg;
		
		if (asprintf(&msg, "%s has left...", client->nickname) >= 0) {
			ServerSendChannelMessage(ListenerGetServer(client->listener), msg);
			free(msg);
		}
	}
	
	close(client->socket);
	
	// This will probbably release this client
	PollDescritptorRemove(client->pollDescriptor);
}

static void ClientDoRead(Client client)
{
	// Make room in the buffer, if non avaiable
	if (client->inBufferLength - client->inBufferFilled < 2 /* something + \0 */) {
		client->inBufferLength *= 2;
		if (client->inBufferLength < 1024)
			client->inBufferLength = 1024;
		client->inBuffer = realloc(client->inBuffer, client->inBufferLength);
		if (client->inBuffer == NULL) {
			perror("realloc");
			ClientDisconnect(client, NULL);
			return;
		}
		
		// Initialize the new buffer to zero
		memset(client->inBuffer+client->inBufferFilled, 0, client->inBufferLength - client->inBufferFilled);
	}
	
	ssize_t len = recv(client->socket, client->inBuffer+client->inBufferFilled, client->inBufferLength - client->inBufferFilled - 1 /* \0 at end */, 0);
	
	if (len < 0) {
		perror("recv");
		ClientDisconnect(client, NULL);
		return;
	}
	else if (len == 0) {
		ClientDisconnect(client, "Client disconnected");
		return;
	}
	else {
		client->inBufferFilled += len;
		
		// Do we have a \n?
		char* newlineSeparator = strchr(client->inBuffer, '\n');
		if (newlineSeparator) { // Yes, handle line
			//
			// The Contract with ClientHandleLine
			//
			// We terminate the line with an \0, it can
			// do any thing with that line as long it stays before the \0
			// If it need to preserve anything, it has to copy it.
			//
			*newlineSeparator = '\0';
			ClientHandleLine(client, client->inBuffer);
			
			// Do we have anything after that \n we found
			if (strlen(newlineSeparator + 1) > 0) {// Yes move it to the beginning of the buffer
				client->inBufferFilled -= newlineSeparator + 1 - client->inBuffer; // Deaccount for the full line
				memmove(client->inBuffer, newlineSeparator + 1, client->inBufferFilled); // Move remaining to beginning
				memset(client->inBuffer+client->inBufferFilled, 0, client->inBufferLength - client->inBufferFilled); // Clear rest
			}
			else { // No, discard the buffer
				client->inBufferFilled = client->inBufferLength = 0;
				free(client->inBuffer);
				client->inBuffer = NULL;
			}
		}
	}
}

static void ClientDoWrite(Client client)
{
	ssize_t len;
	
	len = send(client->socket, client->outBuffer, client->outBufferFilled, 0);
	
	if (len < 0) {
		perror("send");
		ClientDisconnect(client, "Error...");
	}
	else if (len == 0) {
		ClientDisconnect(client, "Client disconnected...");
	}
	else {
		// Everything was send, so discard buffer
		if (client->outBufferFilled == len) {
			client->outBufferFilled = client->outBufferLength = 0;
			free(client->outBuffer);
			client->outBuffer = NULL;
			
			// Don't want to send anymore
			PollDescriptorRemoveEvent(client->pollDescriptor, POLLOUT);
		}
		else {
			memmove(client->outBuffer, client->outBuffer+len, client->outBufferFilled - len);
			// Don't shrink the buffer, only deallocated as seen above.
		}
	}
}

static void ClientHandleLine(Client client, char* line)
{
	// Ignore empty lines
	if (strlen(line) == 0)
		return;
	
	// Is a command
	if (*line == '/') {
		line++; // 'remove' the /
		char* command = strsep(&line, " ");
		char* args = line;
		bool foundCommand = false;
		
		for (uint32_t i = 0; CommandDefs[i].name != NULL; i++) {
			if (strcmp(CommandDefs[i].name, command) == 0) {
				CommandDefs[i].handler(client, args);
				foundCommand = true;
				break;
			}
		}
		
		if (!foundCommand) {
			ClientWriteLine(client, "Unkown command.");
		}
	}
	// Regular line
	else {
		// Construct channel message
		char* msg;
		
		if (asprintf(&msg, "%s: %s", client->nickname, line) > 0) {
			ServerSendChannelMessage(ListenerGetServer(client->listener), msg);
			free(msg);
		} 
	}
}

static bool ClientSetNickname(Client client, char* _nick)
{
	// Copy nick
	char* nickname = malloc(strlen(_nick) + 1 /* \0 */);
	if (nickname == NULL) {
		perror("malloc");
		return false;
	}
	strcpy(nickname, _nick);
	// Normalize nickname
	nickname = strtrim(nickname);
	
	if (nickname == NULL || strlen(nickname) == 0) {
		return false;
	}
	
	// Is already set?
	if (client->nickname && strcmp(client->nickname, nickname) == 0) {
		return true; // Silently succeed
	}
	
	Dictionary clients = ServerGetClients(ListenerGetServer(client->listener));
	
	Lock(clients);
	if (DictionaryGet(clients, nickname) != NULL) {
		Unlock(clients);
		
		return false;
	}
	
	if (client->nickname) {
		DictionaryRemove(clients, client->nickname);
		free(client->nickname);	
	}

	client->nickname = nickname;
	DictionarySet(clients, client->nickname, client);
	Unlock(clients);
	
	return true;
}

void ClientWriteLine(Client client, char* line)
{
	if (client->outBufferLength - client->outBufferFilled < strlen(line) + 1 /* \n */) {
		client->outBufferLength += strlen(line) + 1 /* \n */;
		client->outBuffer = realloc(client->outBuffer, client->outBufferLength);
		if (client->outBuffer == NULL) {
			perror("realloc");
			ClientDisconnect(client, NULL);
			return;
		}
	}
	memcpy(client->outBuffer+client->outBufferFilled, line, strlen(line));
	client->outBufferFilled += strlen(line);
	client->outBuffer[client->outBufferFilled++] = '\n';
	
	// Inform that we want to send, if we're not already sending
	PollDescriptorAddEvent(client->pollDescriptor, POLLOUT);
}

static void ClientHandleNickCommand(Client client, char* arg)
{
	if (arg == NULL) {
		ClientWriteLine(client, "-> /nick <name>");
		return;
	}
	
	char* oldnick;
	asprintf(&oldnick, "%s", client->nickname);
	
	if (ClientSetNickname(client, arg)) {
		char* msg;
		
		if (asprintf(&msg, "%s is now %s", oldnick, client->nickname) >= 0) {
			ServerSendChannelMessage(ListenerGetServer(client->listener), msg);
			free(msg);
		} 
	}
	else
		ClientWriteLine(client, "-> This nick is already used.");
	
	free(oldnick);
}
