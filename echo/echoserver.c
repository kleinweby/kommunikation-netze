#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>

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
		
		buffer[size++] = '\0';
	
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
	
		printf("Old buffer %s\n", client->buffer);
		memcpy(client->buffer + strlen(client->buffer), buffer, size);
		printf("New buffer %s\n", client->buffer);
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

void CloseClient(client_t* client) {
	server_t* server = client->server;
	printf("Close client...\n");
	if (client->socket)
		close(client->socket);
	
	for (int i = 0; i < kMaxClients; i++) {
		// Found a spot
		if (server->clients[i] == client) {
			server->clients[i] = NULL;
			break;
		}
	}
	
	free(client);
}

void runloop(server_t* server) {
	struct pollfd pollDescriptors[kMaxClients + 1];
	int socksToHandle;
	
	memset(pollDescriptors, 0, sizeof(struct pollfd)*kMaxClients + 1);
	int pollNumDescriptors = 1;
	pollDescriptors[0].fd = server->socket;
	pollDescriptors[0].events = POLLIN | POLLHUP;
	{
		int i = 1;
		for (int j = 0; j < kMaxClients; j++) {
			if (server->clients[j] != NULL) {
				pollDescriptors[i].fd = server->clients[j]->socket;
				pollDescriptors[i].events = POLLIN | POLLHUP;
				i++;
				pollNumDescriptors = i;
			}
		}
	}
	
	socksToHandle = poll(pollDescriptors, pollNumDescriptors, 10000);

	if (socksToHandle < 0) {
		server->running = false;
		perror("poll");
		return;
	}
	else if (socksToHandle > 0) {
		for (int i = 0; i < pollNumDescriptors; i++) {
			if ((pollDescriptors[i].revents & POLLIN) > 0) {
				if (pollDescriptors[i].fd == server->socket) {
					if (!acceptNewClient(server)) {
						server->running = false;
						return;
					}
				}
				else {
					for (int j = 0; j < kMaxClients; j++) {
						if (server->clients[j] && server->clients[j]->socket == pollDescriptors[i].fd) {
							if (!processClient(server->clients[j])) {
								CloseClient(server->clients[j]);
							}
						}
					}
				}
			}
			if ((pollDescriptors[i].revents & POLLHUP) > 0) {
				if (pollDescriptors[i].fd == server->socket) {
						server->running = false;
						return;
				}
				else {
					for (int j = 0; j < kMaxClients; j++) {
						if (server->clients[j] && server->clients[j]->socket == pollDescriptors[i].fd) {
								CloseClient(server->clients[j]);
						}
					}
				}
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