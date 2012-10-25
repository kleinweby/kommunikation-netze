
#include "http.h"
#include "str_helper.h"
#include "dictionary.h"
#include "helper.h"
#include "workerthread.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>

static const char* kHTTPLineDelimiter = "\r\n";
static const char* kHTTPContentDelimiter = "\r\n\r\n";
static const char* kHTTPHeaderDelimiter = ":";

typedef enum {
	HTTPConnectionReadingRequest,
	HTTPConnectionProcessingRequest
} HTTPConnectionState;

struct _HTTPRequest {
	//
	// The method of the http request
	//
	HTTPMethod method;
	
	//
	// The version of the http request
	//
	HTTPVersion version;
	
	//
	// The path specified in the request
	//
	char* path;
	
	//
	// The headers of the request
	//
	Dictionary headerDictionary;
	
	//
	// Is a pointer to a memory location which
	// contains the initial data, which is now referenced
	// in path, headers etc. Therefore we must free this
	// when the request is destroyed.
	//
	void* inputBakcend;
};

struct _HTTPConnection {
	int socket;
	
	Server server;
	
	HTTPConnectionState state;
	
	struct sockaddr_in info;
	socklen_t infoLength;
	
	void* buffer;
	size_t bufferFilled;
	size_t bufferLength;
};

static void HTTPRequestParse(HTTPRequest request, char* buffer);
static void HTTPRequestParseActionLine(HTTPRequest request, char* line);
static void HTTPRequestParseHeader(HTTPRequest request, char* buffer);
static void HTTPRequestParseHeaderLine(HTTPRequest request, char* line);

static void HTTPConnectionCallback(int revents, void* userInfo);
static void HTTPConnectionReadRequest(HTTPConnection connection);
static void HTTPProcessRequest(HTTPConnection connection);
static void HTTPProcessRequestWorker(void* userInfo);

bool HTTPCanParseBuffer(char* buffer) {
	if (strstr(buffer, kHTTPContentDelimiter))
		return true;
	
	return false;
}

HTTPRequest HTTPRequestCreate(char* buffer)
{
	HTTPRequest request = malloc(sizeof(struct _HTTPRequest));
	
	request->headerDictionary = DictionaryCreate();
	
	HTTPRequestParse(request, buffer);
	
	return request;
}

HTTPMethod HTTPRequestGetMethod(HTTPRequest request)
{
	return request->method;
}

HTTPVersion HTTPRequestGetVersion(HTTPRequest request)
{
	return request->version;
}

const char* HTTPRequestGetHeaderValueForKey(HTTPRequest request, const char* key)
{
	return DictionaryGet(request->headerDictionary, key);
}

void HTTPRequestDestroy(HTTPRequest request)
{
	free(request);
}

static void HTTPRequestParse(HTTPRequest request, char* buffer)
{
	char* actionLine;
	
	actionLine = strsep_ext(&buffer, kHTTPLineDelimiter);
	
	HTTPRequestParseActionLine(request, actionLine);
	HTTPRequestParseHeader(request, buffer);
}

static void HTTPRequestParseActionLine(HTTPRequest request, char* line)
{
	char* methodString;
	char* path;
	char* versionString;
	
	methodString = strsep_ext(&line, " ");
	path = strsep_ext(&line, " ");
	versionString = strsep_ext(&line, " ");
	
	if (strncmp(methodString, "GET", strlen("GET")) == 0)
		request->method = kHTTPMethodGet;
	else
		request->method = kHTTPMethodUnkown;
	
	if (strncmp(versionString, "HTTP/1.0", strlen("HTTP/1.0")) == 0)
		request->version = kHTTPVersion_1_0;
	else if (strncmp(versionString, "HTTP/1.1", strlen("HTTP/1.1")) == 0)
		request->version = kHTTPVersion_1_1;
	else
		request->version = kHTTPVersionUnkown;
}

static void HTTPRequestParseHeader(HTTPRequest request, char* buffer)
{
	char* line = strsep_ext(&buffer, kHTTPLineDelimiter);
	
	while (line != NULL) {
		HTTPRequestParseHeaderLine(request, line);
		line = strsep_ext(&buffer, kHTTPLineDelimiter);
	}
}

static void HTTPRequestParseHeaderLine(HTTPRequest request, char* line)
{
	char* key;
	char* value;
	
	key = strsep_ext(&line, kHTTPHeaderDelimiter);
	if (!line)
		return;
	
	value = strtrim(line);
	
	DictionarySet(request->headerDictionary, key, value);
}

HTTPConnection HTTPConnectionCreate(Server server)
{
	HTTPConnection connection = malloc(sizeof(struct _HTTPConnection));
	
	memset(connection, 0, sizeof(struct _HTTPConnection));
	
	connection->server = server;
	connection->infoLength = sizeof(connection->info);
	connection->socket = accept(ServerGetSocket(server), (struct sockaddr*)&connection->info, &connection->infoLength);
	
	if (connection->socket < 0) {
		HTTPConnectionDestroy(connection);
		return NULL;
	}
	
	setBlocking(connection->socket, false);
	
	printf("New connection from %s...\n", stringFromSockaddrIn(&connection->info));
	
	connection->state = HTTPConnectionReadingRequest;
	WebServerRegisterPollForSocket(ServerGetWebServer(server), connection->socket, POLLIN|POLLHUP, HTTPConnectionCallback, connection);
	
	return connection;
}

void HTTPConnectionDestroy(HTTPConnection connection)
{
	if (connection->buffer) {
		free(connection->buffer);
	}
	
	if (connection->socket) {
		printf("Close connection from %s...\n", stringFromSockaddrIn(&connection->info));
		WebServerUnregisterPollForSocket(ServerGetWebServer(connection->server), connection->socket);
		close(connection->socket);
	}
	
	free(connection);
}

static void HTTPConnectionCallback(int revents, void* userInfo)
{
	HTTPConnection connection = userInfo;
	
	if ((revents & POLLHUP) > 0) {
		HTTPConnectionDestroy(connection);
		return;
	}
	
	switch(connection->state) {
	case HTTPConnectionReadingRequest:
		HTTPConnectionReadRequest(connection);
		break;
	case HTTPConnectionProcessingRequest:
		break;
	}
}

static void HTTPConnectionReadRequest(HTTPConnection connection)
{
	if (!connection->buffer) {
		connection->bufferFilled = 0;
		connection->bufferLength = 255;
		connection->buffer = malloc(connection->bufferLength);
	}
	
	size_t avaiableBuffer;
	ssize_t readBuffer;
	do {
		avaiableBuffer = connection->bufferLength - connection->bufferFilled;
		readBuffer = 0;
		
		// Not a lot of buffer, expand it
		if (avaiableBuffer < 10) {
			connection->bufferLength *= 255;
			connection->buffer = realloc(connection->buffer, connection->bufferLength);
			avaiableBuffer = connection->bufferLength - connection->bufferFilled;
		}
		
		readBuffer = recv(connection->socket, connection->buffer + connection->bufferFilled, avaiableBuffer, 0);
		
		if (readBuffer < 0) {
			perror("recv");
			HTTPConnectionDestroy(connection);
			return;
		}
		else if (readBuffer == 0) {
			printf("Client closed connection...\n");
			HTTPConnectionDestroy(connection);
			return;
		}
		printf("Read %zd\n", readBuffer);
		
		connection->bufferFilled += readBuffer;
	} while (readBuffer == avaiableBuffer);
	
	if (HTTPCanParseBuffer(connection->buffer)) {
		connection->state = HTTPConnectionProcessingRequest;
		WorkerThreadsEnqueue(HTTPProcessRequestWorker, connection);
	}
}

static void HTTPProcessRequest(HTTPConnection connection)
{
	HTTPRequest request = HTTPRequestCreate(connection->buffer);
	
	char* response = "HTTP/1.0 200 OK\r\n\r\nhallo";
	send(connection->socket, response, strlen(response), 0);
	
	HTTPConnectionDestroy(connection);
	HTTPRequestDestroy(request);
}

static void HTTPProcessRequestWorker(void* userInfo)
{
	HTTPProcessRequest(userInfo);
}
