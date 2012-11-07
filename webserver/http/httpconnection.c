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

#include "httpconnection.h"
#include "httprequest.h"
#include "httpresponse.h"

#include "utils/str_helper.h"
#include "utils/dictionary.h"
#include "utils/helper.h"

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
#ifdef LINUX
#include <sys/sendfile.h>
#endif

#ifdef DARWIN
const char* kHTTPDocumentRoot = "/Users/christian/Public/";
#else
const char* kHTTPDocumentRoot = "/home/speich/htdocs";
#endif

typedef enum {
	HTTPConnectionReadingRequest,
	HTTPConnectionProcessingRequest
} HTTPConnectionState;

DEFINE_CLASS(HTTPConnection,
	int socket;
	
	Server server;
	
	HTTPConnectionState state;
	
	struct sockaddr_in6 info;
	socklen_t infoLength;
	
	char* buffer;
	size_t bufferFilled;
	size_t bufferLength;
);

static void HTTPConnectionReadRequest(HTTPConnection connection);
static void HTTPConnectionDealloc(void* ptr);
static void HTTPProcessRequest(HTTPConnection connection);
static void HTTPSendResponse(HTTPConnection connection, HTTPResponse response);

//
// Resolved a path. Path is to be freed when no longer needed.
//
static char* HTTPResolvePath(HTTPRequest request, char* path);

HTTPConnection HTTPConnectionCreate(Server server, int socket, struct sockaddr_in6 info)
{
	assert(server);
	
	HTTPConnection connection = malloc(sizeof(struct _HTTPConnection));
	
	memset(connection, 0, sizeof(struct _HTTPConnection));
	
	ObjectInit(connection, HTTPConnectionDealloc);
	
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
		Dispatch(ServerGetProcessingDispatchQueue(connection->server), ^{
			HTTPProcessRequest(connection);
		});
	}
	else {
		// For the block
		PollRegister(ServerGetPoll(connection->server), connection->socket, 
			POLLIN|POLLHUP, 0, ServerGetInputDispatchQueue(connection->server), ^(short revents) {
				if ((revents & POLLHUP) > 0) {
					close(connection->socket);
					connection->socket = 0;
					return;
				}
	
				switch(connection->state) {
				case HTTPConnectionReadingRequest:
					HTTPConnectionReadRequest(connection);
					break;
				case HTTPConnectionProcessingRequest:
					break;
				}
			});
	}
}

static void HTTPProcessRequest(HTTPConnection connection)
{	
	HTTPRequest request;
	HTTPResponse response = HTTPResponseCreate(connection);
	struct stat stat;
	char* resolvedPath;
	
	request = HTTPRequestCreate(connection->buffer);
	connection->buffer = NULL;
	
	// We only support get for now
	if (HTTPRequestGetMethod(request) != kHTTPMethodGet) {
		HTTPResponseSetStatusCode(response, kHTTPErrorNotImplemented);
		HTTPSendResponse(connection, response);
		
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
		HTTPResponseFinish(response);
		perror("lstat");
		
		Release(request);
		Release(response);
		return;
	}
	
	if (!S_ISREG(stat.st_mode)) {
		HTTPResponseSetStatusCode(response, kHTTPBadNotFound);
		HTTPResponseFinish(response);
		printf("not regular\n");
		
		Release(request);
		Release(response);
		return;
	}
	
	HTTPResponseSetStatusCode(response, kHTTPOK);
	//HTTPResponseSetResponseString(response, "Hallo wie geht's?");
	HTTPResponseSetResponseFileDescriptor(response, open(resolvedPath, O_RDONLY));
	HTTPResponseFinish(response);
		
	HTTPSendResponse(connection, response);
	
	Release(request);
	Release(response);
}

static void HTTPSendResponse(HTTPConnection connection, HTTPResponse response)
{
	if (!HTTPResponseSend(response)) {
		PollRegister(ServerGetPoll(connection->server), connection->socket, 
			POLLOUT|POLLHUP, 0, ServerGetOutputDispatchQueue(connection->server), ^(short revents) {
#pragma unused(revents)
				HTTPSendResponse(connection, response);
			});
	}
	else
		HTTPConnectionClose(connection);
}

static char* HTTPResolvePath(HTTPRequest request, char* p)
{
#pragma unused(request)
	char* path = malloc(sizeof(char) * PATH_MAX);
	char* real;
	
	strncpy(path, kHTTPDocumentRoot, PATH_MAX);
	strncat(path, p, PATH_MAX);
	
	real = realpath(path, NULL);
	
	free(path);
	
	return real;
}

ssize_t HTTPConnectionSend(HTTPConnection connection, const void *buffer, size_t length)
{
	return send(connection->socket, buffer, length, 0);
}

bool HTTPConnectionSendFD(HTTPConnection connection, int fd, off_t* offset, size_t length)
{
#ifdef DARWIN
	off_t len = (off_t)length;
	off_t offset = lseek(fd, 0, SEEK_CUR);
	int result;
	
	result = sendfile(fd, connection->socket, offset, &len, NULL, 0);
	
	if (result < 0 && errno != EAGAIN) {
		perror("sendfile");
		return -1;
	}
	
	lseek(fd, len, SEEK_CUR);
	
	if (result < 0 && errno == EAGAIN)
		return false;
	return len != 0;
#else
	ssize_t s;
	
	s = sendfile(connection->socket, fd, offset, length);

	if (s < 0 && errno != EAGAIN) {
		perror("sendfile");
		return -1;
	}
	
	if (s < 0 && errno == EAGAIN)
		return false;
	return s == 0;
#endif
}

void HTTPConnectionClose(HTTPConnection connection)
{
	close(connection->socket);
	connection->socket = -1;
}
