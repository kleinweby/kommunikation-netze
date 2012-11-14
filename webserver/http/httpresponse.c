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

#include "httpresponse.h"

#include "utils/dictionary.h"
#include "utils/helper.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

typedef enum {
	kHTTPResponseSendingNotStarted,
	kHTTPResponseSendingStatusLine,
	kHTTPResponseSendingHeaders,
	kHTTPResponseSendingBody,
	kHTTPResponseSendingComplete
} HTTPResponseSendStatus;

struct _HTTPResponseSendGeneralInfo {
	size_t sentBytes;
};

struct _HTTPResponseSendStatusLineInfo {
	struct _HTTPResponseSendGeneralInfo gernal;
	char* statusLine;
};

struct _HTTPResponseSendBodyInfo {
	struct _HTTPResponseSendGeneralInfo gernal;
	off_t fileOffset;
	size_t fileSize;
};

typedef enum {
	kHTTPResponseHeadersSendKey,
	kHTTPResponseSendersSendKeyValueDelimiter,
	kHTTPResponseHeadersSendValue,
	kHTTPResponseHeadersSendLineDelimiter,
	kHTTPResponseHeadersSendHeadersEnd
} HTTPResponseHeadersSendStatus;

struct _HTTPResponseSendHeadersInfo {
	struct _HTTPResponseSendGeneralInfo general;
	
	HTTPResponseHeadersSendStatus status;
	DictionaryIterator iter;
};

DEFINE_CLASS(HTTPResponse,
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
	
	//
	// Manage sending
	//
	HTTPResponseSendStatus sendStatus;
	//
	// Extra status for sending by the current step
	//
	void* sendStatusExtra;
);

// Convienience method for an error condition
static void HTTPResponseDealloc(void* ptr);

//
// If returns false, you need to call this method
// again when the socket is able to write data
//
static bool HTTPResponseSendStatusLine(HTTPResponse response);
static bool HTTPResponseSendHeaders(HTTPResponse response);
static bool HTTPResponseSendBody(HTTPResponse response);

static bool HTTPResponseSendString(HTTPResponse response, const char* string);

HTTPResponse HTTPResponseCreate(HTTPConnection connection)
{
	HTTPResponse response = malloc(sizeof(struct _HTTPResponse));
	
	memset(response, 0, sizeof(struct _HTTPResponse));
	
	ObjectInit(response, HTTPResponseDealloc);
	
	response->connection = connection;
	response->headerDictionary = DictionaryCreate();
	
	HTTPResponseSetHeaderValue(response, "Server", "webserver/dev");
	
	return response;
}

void HTTPResponseDealloc(void* ptr)
{
	HTTPResponse response = ptr;
	
	if (response->responseFileDescriptor > 0)
		close(response->responseFileDescriptor);
	
	Release(response->headerDictionary);
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

bool HTTPResponseSend(HTTPResponse response)
{
	switch(response->sendStatus) {
	case kHTTPResponseSendingNotStarted:
		response->sendStatus++;
	case kHTTPResponseSendingStatusLine:
		if (!HTTPResponseSendStatusLine(response))
			return false;
		response->sendStatus++;
	case kHTTPResponseSendingHeaders:
		if (!HTTPResponseSendHeaders(response))
			return false;
		response->sendStatus++;
	case kHTTPResponseSendingBody:
		if (!HTTPResponseSendBody(response))
			return false;
		response->sendStatus++;
	case kHTTPResponseSendingComplete:
		return true;
	}
}

static bool HTTPResponseSendStatusLine(HTTPResponse response)
{
	struct _HTTPResponseSendStatusLineInfo* info = response->sendStatusExtra;
	
	if (!info) {
		response->sendStatusExtra = malloc(sizeof(struct _HTTPResponseSendStatusLineInfo));
		
		memset(response->sendStatusExtra, 0, sizeof(struct _HTTPResponseSendStatusLineInfo));
		
		info = response->sendStatusExtra;
		
		uint maxLineLength = 255;
		info->statusLine = malloc(sizeof(char) * maxLineLength);
		snprintf(info->statusLine, maxLineLength, "HTTP/1.0 %3d %s\r\n", response->code, HTTPStatusNameFromCode(response->code));
	}
	
	if (!HTTPResponseSendString(response, info->statusLine))
		return false;

	free(response->sendStatusExtra);
	response->sendStatusExtra = NULL;
	return true;
}

static bool HTTPResponseSendHeaders(HTTPResponse response)
{
	struct _HTTPResponseSendHeadersInfo* info = response->sendStatusExtra;
	
	if (!info) {
		response->sendStatusExtra = malloc(sizeof(struct _HTTPResponseSendHeadersInfo));
		
		memset(response->sendStatusExtra, 0, sizeof(struct _HTTPResponseSendHeadersInfo));
		
		info = response->sendStatusExtra;
		
		info->status = kHTTPResponseHeadersSendKey;
		info->iter = DictionaryGetIterator(response->headerDictionary);
	}
	
	while(DictionaryIteratorGetKey(info->iter) != NULL) {
		switch(info->status) {
		case kHTTPResponseHeadersSendKey:
		{
			char* key = DictionaryIteratorGetKey(info->iter);
			
			if (!HTTPResponseSendString(response, key))
				return false;

			info->status++;
		}
			break;
		case kHTTPResponseSendersSendKeyValueDelimiter:
		{
			char* del = ": ";
			
			if (!HTTPResponseSendString(response, del))
				return false;

			info->status++;
		}
			break;
		case kHTTPResponseHeadersSendValue:
		{
			char* value = DictionaryIteratorGetValue(info->iter);
			
			if (!HTTPResponseSendString(response, value))
				return false;

			info->status++;
		}
			break;
		case kHTTPResponseHeadersSendLineDelimiter:
		{
			char* del = "\r\n";
			
			if (!HTTPResponseSendString(response, del))
				return false;

			if (DictionaryIteratorNext(info->iter)) {
				info->status = kHTTPResponseHeadersSendKey;
			}
			else {
				info->status++;
			}
		}
			break;
		case kHTTPResponseHeadersSendHeadersEnd:
			break;
		};
	}
	
	if (info->status == kHTTPResponseHeadersSendHeadersEnd) {
		char* del = "\r\n";
			
		if (!HTTPResponseSendString(response, del))
			return false;

		info->status++;
		free(response->sendStatusExtra);
		response->sendStatusExtra = NULL;
	}
	
	return true;
}

static bool HTTPResponseSendBody(HTTPResponse response)
{
	struct _HTTPResponseSendBodyInfo* info = response->sendStatusExtra;
	
	if (!info) {
		response->sendStatusExtra = malloc(sizeof(struct _HTTPResponseSendBodyInfo));
		
		memset(response->sendStatusExtra, 0, sizeof(struct _HTTPResponseSendBodyInfo));
		
		info = response->sendStatusExtra;
		
		if (response->responseFileDescriptor) {
			struct stat stat;
			
			if (fstat(response->responseFileDescriptor, &stat) < 0) {
				perror("fstat");
			}
			info->fileSize = (size_t)stat.st_size;
		}
	}

	if (response->responseString) {
		if (!HTTPResponseSendString(response, response->responseString))
			return false;
	}
	else if (response->responseFileDescriptor) {
		if (!HTTPConnectionSendFD(response->connection, response->responseFileDescriptor, &info->fileOffset, info->fileSize - (size_t)info->fileOffset))
			return false;
	}

	return true;
}

void HTTPResponseFinish(HTTPResponse response)
{
#pragma unused(response)
}

static bool HTTPResponseSendString(HTTPResponse response, const char* string)
{
	struct _HTTPResponseSendGeneralInfo* info = response->sendStatusExtra;
	ssize_t s;
			
	s = HTTPConnectionSend(response->connection, string + info->sentBytes,
				strlen(string) - info->sentBytes);
			
	if (s < 0) {
		perror("send");
		return false;
	}
			
	info->sentBytes += (size_t)s;
			
	if (info->sentBytes < strlen(string)) {
		return false;
	}
	
	info->sentBytes = 0;
	return true;
}
