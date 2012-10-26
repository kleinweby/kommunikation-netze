
#include "http.h"
#include "str_helper.h"
#include "dictionary.h"
#include "helper.h"
#include "retainable.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>

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

static char* kHTTPConnectionMagic = "_HTTPConnection";
static char* kHTTPConnectionDeadMagic = ":(";

struct _HTTPConnection {
	Retainable retainable;
	
	char *magic;
	int socket;
	
	Server server;
	
	HTTPConnectionState state;
	
	struct sockaddr_in info;
	socklen_t infoLength;
	
	void* buffer;
	size_t bufferFilled;
	size_t bufferLength;
};

struct _HTTPResponse {
	//
	// The connection this resposne is accosiated to
	//
	HTTPConnection connection;
	
	//
	// The http status code to send
	//
	HTTPStatusCode code;
	
	//
	// Headers
	//
	Dictionary headerDictionary;
	
	//
	// Response
	// one of the following is valid
	//
	
	char* responseString;
	int responseFileDescriptor;
};

static void HTTPRequestParse(HTTPRequest request, char* buffer);
static void HTTPRequestParseActionLine(HTTPRequest request, char* line);
static void HTTPRequestParseHeader(HTTPRequest request, char* buffer);
static void HTTPRequestParseHeaderLine(HTTPRequest request, char* line);

static void HTTPConnectionReadRequest(HTTPConnection connection);
static void HTTPProcessRequest(HTTPConnection connection);

// Convienience method for an error condition
static void HTTPResponseSendError(HTTPConnection connection, HTTPStatusCode code, char* responseString);

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

HTTPConnection HTTPConnectionCreate(Server server, int socket, struct sockaddr info)
{
	assert(server);
	
	HTTPConnection connection = malloc(sizeof(struct _HTTPConnection));
	
	memset(connection, 0, sizeof(struct _HTTPConnection));
	
	RetainableInitialize(&connection->retainable, (void (*)(void *))HTTPConnectionDestroy);
	connection->magic = kHTTPConnectionMagic;
	
	connection->server = server;
	memcpy(&connection->info, &info, sizeof(struct sockaddr_in));
	connection->socket = socket;
	connection->infoLength = sizeof(connection->info);
	
	setBlocking(connection->socket, false);
	
	printf("New connection:%p from %s...\n", connection, stringFromSockaddrIn(&connection->info));
	
	connection->state = HTTPConnectionReadingRequest;

	// For the block
	Retain(connection);
	PollRegister(ServerGetPoll(connection->server), connection->socket, 
		POLLIN|POLLHUP, 0, ServerGetInputDispatchQueue(connection->server), ^(int revents) {
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
	
	
	// No release
	return connection;
}

void HTTPConnectionDestroy(HTTPConnection connection)
{
	assert(connection->magic == kHTTPConnectionMagic);
	if (connection->buffer) {
		free(connection->buffer);
	}
	
	if (connection->socket) {
		printf("Close connection:%p from %s...\n", connection, stringFromSockaddrIn(&connection->info));
		PollUnregister(ServerGetPoll(connection->server), connection->socket);
		close(connection->socket);
	}
	
	printf("Dealloc connection:%p\n", connection);
	connection->magic = kHTTPConnectionDeadMagic;
	free(connection);
}

static void HTTPConnectionReadRequest(HTTPConnection connection)
{
	assert(connection->magic == kHTTPConnectionMagic);
	
	if (!connection->buffer) {
		connection->bufferFilled = 0;
		connection->bufferLength = 255;
		connection->buffer = malloc(connection->bufferLength);
	}
	
	assert(connection->buffer);
	
	ssize_t avaiableBuffer;
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
		
		assert(connection->bufferFilled + readBuffer <= connection->bufferLength);
		
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
		
		connection->bufferFilled += readBuffer;
	} while (readBuffer == avaiableBuffer);
	
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
			POLLIN|POLLHUP, 0, ServerGetInputDispatchQueue(connection->server), ^(int revents) {
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
	assert(connection->magic == kHTTPConnectionMagic);
	
	HTTPRequest request = HTTPRequestCreate(connection->buffer);
	
	/*if (request->method != kHTTPMethodGet) {
		HTTPResponseSendError(connection, kHTTPErrorNotImplemented, "Unkown method");
		return;
	}*/
	
	char* response = "HTTP/1.0 200 OK\r\n\r\nhallo";
	setBlocking(connection->socket, true);
	send(connection->socket, response, strlen(response),0);
	HTTPResponseSendError(connection, kHTTPOK, response);
	
	close(connection->socket);
	//HTTPConnectionDestroy(connection);
	HTTPRequestDestroy(request);
}

HTTPResponse HTTPResponseCreate(HTTPConnection connection)
{
	HTTPResponse response = malloc(sizeof(struct _HTTPResponse));
	
	memset(response, 0, sizeof(struct _HTTPResponse));
	
	return response;
}

void HTTPResponseDestroy(HTTPResponse response)
{
	free(response);
}

void HTTPResponseSetStatusCode(HTTPResponse response, HTTPStatusCode code)
{
	response->code = code;
}

void HTTPResponseSetHeaderValue(HTTPResponse response, const char* key, const char* value)
{
	DictionarySet(response->headerDictionary, key, value);
}

void HTTPResponseSetResponseString(HTTPResponse response, char* string)
{
	response->responseString = string;
}

void HTTPResponseSetResponseFileDescriptor(HTTPResponse response, int fd)
{
	response->responseFileDescriptor = fd;
}

static void HTTPResponseSendError(HTTPConnection connection, HTTPStatusCode code, char* responseString)
{
	HTTPResponse response = HTTPResponseCreate(connection);
	
	HTTPResponseSetResponseString(response, responseString);
	
	HTTPResponseDestroy(response);
}

void HTTPResponseSendComplete(HTTPResponse response)
{
	setBlocking(response->connection->socket, true);
	
	
	
	setBlocking(response->connection->socket, false);
}
