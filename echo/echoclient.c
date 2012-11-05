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

#include "helper.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

char *readline (const char *prompt);

int connectToServer(const char* host, const char* port) {
	int sock = -1;
    int error;
	struct addrinfo *result;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
    error = getaddrinfo(host, port, &hints, &result);
	if (error != 0) {
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
			char* desc = stringFromSockaddrIn(((struct sockaddr_in const *)server->ai_addr));
			
			if (desc) {
				printf("Try connecting to %s...\n", desc);
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
	
	return sock;
}

void runloop(int socket) {
	char buffer[255];
	char* input;
		
	input = readline("> ");
	
	if (!input) {
		close(socket);
		exit(EXIT_SUCCESS);
	}

	if (send(socket, input, strlen(input), 0) < 0) {
		perror("send");
		close(socket);
		exit(EXIT_FAILURE);
	}
	if (send(socket, "\n", 1, 0) < 0) {
		perror("send");
		close(socket);
		exit(EXIT_FAILURE);
	}
	free(input);
	
	memset(buffer, 0, 255);
	int i = 0;

	while(i == 0 || (strstr(buffer, "\n") == NULL && i < 254)) {
		ssize_t s = recv(socket, buffer+i, 254-i, 0);
		
		if (s < 0) {
			perror("recv");
			close(socket);
			exit(EXIT_FAILURE);
		}
		
		i += s;
	}
	
	buffer[i] = '\0';
	
	printf("< %s", buffer);
}

int main(int argc, char** argv) {
	const char* host;
	const char* port;
	int socket;
	
	if (argc != 3) {
		printf("%s host port\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	host = argv[1];
	port = argv[2];
	
	socket = connectToServer(host, port);
	
	if (socket < 0) {
		printf("Could not connect to any server.\n");
		return EXIT_FAILURE;
	}
	
	while(true)
		runloop(socket);
			
	return EXIT_SUCCESS;
}

char* readline(const char* prompt) {
	printf("%s", prompt);
	fflush(stdout);
	
	char* buffer = malloc(20);
	size_t size = 20;
	int i = 0;
	
	if (buffer == NULL) {
		perror("malloc");
		return NULL;
	}
	
	for(;;) {
		char c = fgetc(stdin);
		
		if (c == EOF) {
			free(buffer);
			return NULL;
		}
		if (c == '\n')
			break;
		
		if (i >= size) {
			size *= 2;
			buffer = realloc(buffer, size);
			
			if (buffer == NULL) {
				perror("realloc");
				return NULL;
			}
		}
		buffer[i] = c;
		i++;
	}
	
	buffer[i] = '\0';

	return buffer;
}
