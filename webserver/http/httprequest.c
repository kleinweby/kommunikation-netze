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

#include "httprequest.h"

#include "utils/str_helper.h"
#include "utils/dictionary.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

DEFINE_CLASS(HTTPRequest,
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
	void* inputBackend;
);

static bool HTTPRequestParse(HTTPRequest request, char* buffer);
static bool HTTPRequestParseActionLine(HTTPRequest request, char* line);
static bool HTTPRequestParseHeader(HTTPRequest request, char* buffer);
static bool HTTPRequestParseHeaderLine(HTTPRequest request, char* line);
static void HTTPRequestDealloc(void* ptr);

bool HTTPCanParseBuffer(char* buffer) {
	assert(buffer != NULL);
	
	if (strstr(buffer, kHTTPContentDelimiter))
		return true;
		
	if (strstr(buffer, "\n\n"))
		return true;
	
	return false;
}

HTTPRequest HTTPRequestCreate(char* buffer)
{
	assert(buffer != NULL);
	
	HTTPRequest request = malloc(sizeof(struct _HTTPRequest));
	
	if (request == NULL) {
		perror("malloc");
		return NULL;
	}
	
	memset(request, 0, sizeof(struct _HTTPRequest));
	
	ObjectInit(request, HTTPRequestDealloc);
	
	request->headerDictionary = DictionaryCreate();
	
	if (request->headerDictionary == NULL) {
		printf("Could not create header dictionary.\n");
		Release(request);
		return NULL;
	}
	
	request->inputBackend = buffer;
	
	if (!HTTPRequestParse(request, buffer)) {
		Release(request);
		return NULL;
	}
	
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

void HTTPRequestDealloc(void* ptr)
{
	HTTPRequest request = ptr;
	
	if (request->inputBackend)
		free(request->inputBackend);
	
	Release(request->headerDictionary);
	free(request);
}

static bool HTTPRequestParse(HTTPRequest request, char* buffer)
{
	char* actionLine;
	
	actionLine = strsep_ext(&buffer, kHTTPLineDelimiter);
	
	if (!HTTPRequestParseActionLine(request, actionLine))
		return false;
	if (!HTTPRequestParseHeader(request, buffer))
		return false;
		
	return true;
}

static bool HTTPRequestParseActionLine(HTTPRequest request, char* line)
{
	char* methodString;
	char* versionString;
	
	methodString = strsep_ext(&line, " ");
	request->path = strsep_ext(&line, " ");
	versionString = strsep_ext(&line, " ");
	
	if (methodString == NULL)
		return false;
		
	if (strncmp(methodString, "GET", strlen("GET")) == 0)
		request->method = kHTTPMethodGet;
	else
		request->method = kHTTPMethodUnkown;
	
	if (versionString == NULL)
		return false;
		
	if (strncmp(versionString, "HTTP/1.0", strlen("HTTP/1.0")) == 0)
		request->version = kHTTPVersion_1_0;
	else if (strncmp(versionString, "HTTP/1.1", strlen("HTTP/1.1")) == 0)
		request->version = kHTTPVersion_1_1;
	else
		request->version = kHTTPVersionUnkown;
		
	return true;
}

static bool HTTPRequestParseHeader(HTTPRequest request, char* buffer)
{
	char* line = strsep_ext(&buffer, kHTTPLineDelimiter);
	
	while (line != NULL) {
		if (!HTTPRequestParseHeaderLine(request, line))
			return false;
		line = strsep_ext(&buffer, kHTTPLineDelimiter);
	}
	
	return true;
}

static bool HTTPRequestParseHeaderLine(HTTPRequest request, char* line)
{
	char* key;
	char* value;
	
	key = strsep_ext(&line, kHTTPHeaderDelimiter);
	if (!line)
		return false;
	
	value = strtrim(line);
	
	DictionarySet(request->headerDictionary, key, value);
	
	return true;
}

char* HTTPRequestGetPath(HTTPRequest request)
{
	return request->path;
}
