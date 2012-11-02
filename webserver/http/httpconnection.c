
#include "httpconnection.h"
#include "httprequest.h"
#include "httpresponse.h"

#include "utils/str_helper.h"
#include "utils/dictionary.h"
#include "utils/helper.h"
#include "utils/retainable.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>

typedef enum {
	HTTPConnectionReadingRequest,
	HTTPConnectionProcessingRequest
} HTTPConnectionState;

struct _HTTPConnection {
	Retainable retainable;
	
	int socket;
	
	Server server;
	
	HTTPConnectionState state;
	
	struct sockaddr_in6 info;
	socklen_t infoLength;
	
	char* buffer;
	size_t bufferFilled;
	size_t bufferLength;
};

static void HTTPConnectionReadRequest(HTTPConnection connection);
static void HTTPConnectionDealloc(void* ptr);
static void HTTPProcessRequest(HTTPConnection connection);

//
// Resolved a path. Path is to be freed when no longer needed.
//
static char* HTTPResolvePath(HTTPRequest request, char* path);

HTTPConnection HTTPConnectionCreate(Server server, int socket, struct sockaddr_in6 info)
{
	assert(server);
	
	HTTPConnection connection = malloc(sizeof(struct _HTTPConnection));
	
	memset(connection, 0, sizeof(struct _HTTPConnection));
	
	RetainableInitialize(&connection->retainable, HTTPConnectionDealloc);
	
	connection->server = server;
	memcpy(&connection->info, &info, sizeof(struct sockaddr_in6));
	connection->socket = socket;
	connection->infoLength = sizeof(connection->info);
	
	setBlocking(connection->socket, false);
	
	printf("New connection:%p from %s...\n", connection, stringFromSockaddrIn(&connection->info));
	
	connection->state = HTTPConnectionReadingRequest;

	// For the block
	HTTPConnectionReadRequest(connection);
	// Retain(connection);
	// 	PollRegister(ServerGetPoll(connection->server), connection->socket, 
	// 		POLLIN|POLLHUP, 0, ServerGetInputDispatchQueue(connection->server), ^(short revents) {
	// 			if ((revents & POLLHUP) > 0) {
	// 				close(connection->socket);
	// 				connection->socket = 0;
	// 				Release(connection);
	// 				return;
	// 			}
	// 	
	// 			switch(connection->state) {
	// 			case HTTPConnectionReadingRequest:
	// 				HTTPConnectionReadRequest(connection);
	// 				break;
	// 			case HTTPConnectionProcessingRequest:
	// 				break;
	// 			}
	// 			Release(connection);
	// 		});
	
	
	// No release
	return connection;
}

void HTTPConnectionDealloc(void* ptr)
{
	HTTPConnection connection = ptr;
			
	if (connection->buffer) {
		free(connection->buffer);
	}
	
	if (connection->socket) {
		printf("Close connection:%p from %s...\n", connection, stringFromSockaddrIn(&connection->info));
		PollUnregister(ServerGetPoll(connection->server), connection->socket);
		close(connection->socket);
	}
	
	printf("Dealloc connection:%p\n", connection);
	free(connection);
}

static void HTTPConnectionReadRequest(HTTPConnection connection)
{	
	if (!connection->buffer) {
		connection->bufferFilled = 0;
		connection->bufferLength = 255;
		connection->buffer = malloc(connection->bufferLength);
	}
	
	assert(connection->buffer);
	
	size_t avaiableBuffer;
	ssize_t readBuffer;
	do {
		avaiableBuffer = connection->bufferLength - connection->bufferFilled;
		readBuffer = 0;
		
		// Not a lot of buffer, expand it
		if (avaiableBuffer < 10) {
			connection->bufferLength *= 2;
			connection->buffer = realloc(connection->buffer, connection->bufferLength);
			avaiableBuffer = connection->bufferLength - connection->bufferFilled;
		}
		
		assert(connection->buffer);
		
		readBuffer = recv(connection->socket, connection->buffer + connection->bufferFilled, avaiableBuffer, 0);
				
		if (readBuffer < 0) {
			if (errno != EAGAIN) {
				perror("recv");
				Release(connection);
				return;
			}
			readBuffer = 0;
		}
		else if (readBuffer == 0) {
			printf("Client closed connection...\n");
			Release(connection);
			return;
		}
		
		assert(connection->bufferFilled + (size_t)readBuffer <= connection->bufferLength);
		
		connection->bufferFilled += (size_t)readBuffer;
	} while ((size_t)readBuffer == avaiableBuffer);
	
	if (HTTPCanParseBuffer(connection->buffer)) {
		connection->state = HTTPConnectionProcessingRequest;
		
		// For the block
		Retain(connection);
		Dispatch(ServerGetProcessingDispatchQueue(connection->server), ^{
			HTTPProcessRequest(connection);
			Release(connection);
		});
	}
	else {
		// For the block
		Retain(connection);
		PollRegister(ServerGetPoll(connection->server), connection->socket, 
			POLLIN|POLLHUP, 0, ServerGetInputDispatchQueue(connection->server), ^(short revents) {
				if ((revents & POLLHUP) > 0) {
					close(connection->socket);
					connection->socket = 0;
					Release(connection);
					return;
				}
	
				switch(connection->state) {
				case HTTPConnectionReadingRequest:
					HTTPConnectionReadRequest(connection);
					break;
				case HTTPConnectionProcessingRequest:
					break;
				}
				Release(connection);
			});
	}
}

static void HTTPProcessRequest(HTTPConnection connection)
{	
	HTTPRequest request = HTTPRequestCreate(connection->buffer);
	HTTPResponse response = HTTPResponseCreate(connection);
	struct stat stat;
	char* resolvedPath;
	
	// We only support get for now
	if (HTTPRequestGetMethod(request) != kHTTPMethodGet) {
		HTTPResponseSetStatusCode(response, kHTTPErrorNotImplemented);
		HTTPResponseSendComplete(response);
		HTTPResponseFinish(response);
		
		Release(request);
		Release(response);
		return;
	}
	
	resolvedPath = HTTPResolvePath(request, HTTPRequestGetPath(request));
	
	printf("Real path: %s\n", resolvedPath);
	
	// Check the file
	if (lstat(resolvedPath, &stat) < 0) {
		HTTPResponseSetStatusCode(response, kHTTPBadNotFound);
		HTTPResponseSendComplete(response);
		HTTPResponseFinish(response);
		perror("lstat");
		
		Release(request);
		Release(response);
		return;
	}
	
	if (!S_ISREG(stat.st_mode)) {
		HTTPResponseSetStatusCode(response, kHTTPBadNotFound);
		HTTPResponseSendComplete(response);
		HTTPResponseFinish(response);
		printf("not regular\n");
		
		Release(request);
		Release(response);
		return;
	}
	
	HTTPResponseSetStatusCode(response, kHTTPOK);
	//HTTPResponseSetResponseString(response, "Hallo wie geht's?");
	HTTPResponseSetResponseFileDescriptor(response, open(resolvedPath, O_RDONLY));
	HTTPResponseSendComplete(response);
	HTTPResponseFinish(response);
		
	Release(request);
	Release(response);
}

static char* HTTPResolvePath(HTTPRequest request, char* p)
{
#pragma unused(request)
	char* path = malloc(sizeof(char) * PATH_MAX);
	char* real;
	
	strncpy(path, "/Users/christian/Public/", PATH_MAX);
	strncat(path, p, PATH_MAX);
	
	real = realpath(path, NULL);
	
	free(path);
	
	return real;
}

ssize_t HTTPConnectionSend(HTTPConnection connection, const void *buffer, size_t length)
{
	return send(connection->socket, buffer, length, 0);
}

ssize_t HTTPConnectionSendFD(HTTPConnection connection, int fd, size_t length)
{
#ifdef DARWIN
	off_t len = (off_t)length;
	off_t offset = lseek(fd, 0, SEEK_CUR);
	
	if (sendfile(fd, connection->socket, offset, &len, NULL, 0) < 0) {
		perror("sendfile");
		return -1;
	}
	
	lseek(fd, len, SEEK_CUR);
	
	printf("sent %lld\n", len);
	return len;
#else
#error Linux not supported yet
#endif
}

void HTTPConnectionClose(HTTPConnection connection)
{
	close(connection->socket);
	connection->socket = -1;
}
