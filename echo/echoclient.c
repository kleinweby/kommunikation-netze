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
	int sock;
	struct addrinfo *result;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if (getaddrinfo(host, port, &hints, &result) < 0) {
		perror("getaddrinfo");
		return -1;
	}
	
	// Cylce through the results an try to connect
	for (struct addrinfo* server = result; server != NULL; server = server->ai_next) {
		sock = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	
		if (sock < 0) {
			continue;
		}
		
		{
			char buffer[255];
			if (inet_ntop(server->ai_family, &((struct sockaddr_in const *)server->ai_addr)->sin_addr, buffer, 255)) {
				printf("Try connecting to %s...\n", buffer);
			}
		}
		
		if (connect(sock, server->ai_addr, server->ai_addrlen) < 0) {
			close(sock);
			sock = -1;
		} 
		else
			break;
	}	
	
	return sock;
}

void runloop(int socket) {
	char buffer[255];
	char* input;
		
	input = readline(">");
	
	if (!input)
		exit(EXIT_SUCCESS);

	send(socket, input, strlen(input), 0);
	send(socket, "\n", 1, 0);
	free(input);
	
	printf("<");
	memset(buffer, 0, 255);
	int i = 0;

	while(i == 0 || (strstr(buffer, "\n") == NULL && i < 255)) {
		ssize_t s = recv(socket, buffer+i, 255-i, 0);
		
		if (s < 0)
			perror("recv");
		
		i += s;
	}
	
	buffer[i] = '\0';
	
	printf("%s", buffer);
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
	int i = 0;
	
	for(;;) {
		char c = fgetc(stdin);
		
		if (c == EOF) {
			free(buffer);
			return NULL;
		}
		if (c == '\n')
			break;
		
		if (i > sizeof(buffer)) {
			buffer = realloc(buffer, sizeof(buffer)*2);
		}
		buffer[i] = c;
		i++;
	}
	
	buffer[i] = '\0';

	return buffer;
}