#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>

static const int kMaxClients = 10;

typedef struct client client_t;
typedef struct server server_t;

struct server {
	// The socket we listen on for new clients
	int socket;
	// The port we listen on
	uint16_t port;
	
	// Is true when the server should be kept running
	bool running;
	
	// The set for sockets that we observe
	fd_set selectSockets;
	
	// Holds some data about the connected clients
	client_t* clients[kMaxClients];
};

struct client {
	server_t* server;
	
	struct sockaddr_in info;
	socklen_t infoLength;
	int socket;
	
	char* buffer;
	size_t bufferLen;
};

bool setBlocking(int socket, bool blocking) {
	int opts;
	
	opts = fcntl(socket, F_GETFL);
	if (opts < 0) {
		perror("fcntl");
		return false;
	}
	
	if (blocking)
		opts &= ~O_NONBLOCK;
	else
		opts |= O_NONBLOCK;
	
	if (fcntl(socket, F_SETFL, opts) < 0) {
		perror("fcntl");
		return false;
	}
	
	return true;
}

// Open and bind a socket on port
//  - port = 0 means that we let the kernel assign one for us
server_t* openAndBindServerSocket(int port) {
	server_t* server = malloc(sizeof(server_t));
	
	server->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server->socket < 0) {
		free(server);
		return NULL;
	}
	
	{
		struct sockaddr_in addr;
		
		memset(&addr, 0, sizeof(struct sockaddr_in));
		
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		
		if (bind(server->socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			close(server->socket);
			free(server);
			return NULL;
		}
		
		// When we did not specify a port, read which port
		// we connected to
		if (port == 0) {
			socklen_t len = sizeof(addr);
			if (getsockname(server->socket, (struct sockaddr*)&addr, &len) < 0) {
				close(server->socket);
				free(server);
				return NULL;
			}
			server->port = ntohs(addr.sin_port);
		}
	}
	
	// Allow up to kMaxClients waiting to accept
	listen(server->socket, kMaxClients);
	
	// Setup the select sockets
	FD_ZERO(&server->selectSockets);
	
	// Add the server socket for incoming connections
	FD_SET(server->socket, &server->selectSockets);
	
	return server;
}

bool acceptNewClient(server_t* server) {
	client_t* client = malloc(sizeof(client_t));
	client->infoLength = sizeof(client->info);
	
	client->socket = accept(server->socket, (struct sockaddr*)&client->info, &client->infoLength);
	
	if (client->socket < 0) {
		free(client);
		perror("accept");
		return false;
	}
	
	if (!setBlocking(client->socket, false)) {
		free(client);
		return false;
	}
	
	// Now put the client into our server
	
	for (int i = 0; i < kMaxClients; i++) {
		// Found a spot
		if (server->clients[i] == NULL) {
			server->clients[i] = client;
			client->server = server;
			break;
		}
	}
	
	if (!client->server) {
		printf("Could not accept client %s. We're full.\n", "");
		const char* msg = "Server full\n";
		send(client->socket, msg, strlen(msg), 0);
		//closeClient(client);
		return false;
	}
	
	printf("Accept client %s...\n", "");
	
	FD_SET(client->socket, &server->selectSockets);

	return true;
}

bool readClientToBuffer(client_t* client) {
	char buffer[255];
	
	ssize_t size;
	
	do {
		size = recv(client->socket, buffer, 255, 0);
	
		if (size < 0) {
			perror("recv");
			return false;
		}
	
		if (size == 0)
			break;
		
		buffer[size] = '\0';
	
		if (client->buffer == NULL) {
			client->buffer = malloc(20);
			client->bufferLen = 20;
			memset(client->buffer, 0, client->bufferLen);
		}
	
		// Need resizing
		if (strlen(client->buffer) + size > client->bufferLen) {
			size_t newSize = client->bufferLen;
		
			while (newSize < strlen(client->buffer) + size) {
				newSize *= 2;
			}
		
			client->buffer = realloc(client->buffer, newSize);
			
			client->bufferLen = newSize;
		}
	
		memcpy(client->buffer + strlen(client->buffer), buffer, size);
	} while(size == 255);
	
	return true;
}

bool writeBufferToClient(client_t* client) {
	ssize_t sendSize = send(client->socket, client->buffer, strlen(client->buffer), 0);
	
	if (sendSize < 0) {
		perror("send");
		return false;
	}
	
	// Now move our buffer up
	memmove(client->buffer, client->buffer + sendSize, strlen(client->buffer) + 1 - sendSize);
	memset(client->buffer + sendSize, 0, client->bufferLen - sendSize);
	
	return true;
}

bool processClient(client_t* client) {
	if (!readClientToBuffer(client))
		return false;
	
	if (client->buffer && strstr(client->buffer, "\n") != NULL) {
		if (!writeBufferToClient(client))
			return false;
	}
	
	return true;
}

void closeClient(client_t* client) {
	
}

void runloop(server_t* server) {
	fd_set fds;
	struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
	int socksToHandle;
	
	// Copy the set so that we don't destory it
	FD_COPY(&server->selectSockets, &fds);
	
	socksToHandle = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
	
	if (socksToHandle < 0) {
		server->running = false;
		perror("select");
		return;
	}
	else if (socksToHandle > 0) {
		if (FD_ISSET(server->socket, &fds)) {
			if (!acceptNewClient(server)) {
				server->running = false;
				return;
			}
			socksToHandle--;
		}
		
		for (int i = 0; i < kMaxClients && socksToHandle > 0; i++) {
			if (FD_ISSET(server->clients[i]->socket, &fds)) {
				if (!processClient(server->clients[i])) {
					closeClient(server->clients[i]);
				}
				
				socksToHandle--;
			}
		} 
	}
}

int main(int argc, char** argv) {
	server_t* server;
	uint16_t desiredPort;
	
	if (argc == 1)
		desiredPort = 0;
	else if (argc == 2)
		desiredPort = atoi(argv[1]);
	else {
		printf("%s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	printf("Opening server socket...\n");
	server = openAndBindServerSocket(desiredPort);
	
	if (!server) {
		perror("Could not open server socket:");
		return EXIT_FAILURE;
	}
	
	printf("Listing on %d for connections...\n", server->port);
	
	server->running = true;
	while(server->running)
		runloop(server);
		
	return EXIT_SUCCESS;
}