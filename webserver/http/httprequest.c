#include "httprequest.h"

#include "utils/str_helper.h"
#include "utils/retainable.h"
#include "utils/dictionary.h"

#include <stdlib.h>
#include <string.h>

struct _HTTPRequest {
	Retainable retainable;
	
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

static void HTTPRequestParse(HTTPRequest request, char* buffer);
static void HTTPRequestParseActionLine(HTTPRequest request, char* line);
static void HTTPRequestParseHeader(HTTPRequest request, char* buffer);
static void HTTPRequestParseHeaderLine(HTTPRequest request, char* line);
static void HTTPRequestDealloc(void* ptr);

bool HTTPCanParseBuffer(char* buffer) {
	if (strstr(buffer, kHTTPContentDelimiter))
		return true;
	
	return false;
}

HTTPRequest HTTPRequestCreate(char* buffer)
{
	HTTPRequest request = malloc(sizeof(struct _HTTPRequest));
	
	memset(request, 0, sizeof(struct _HTTPRequest));
	
	RetainableInitialize(&request->retainable, HTTPRequestDealloc);
	
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

void HTTPRequestDealloc(void* ptr)
{
	HTTPRequest request = ptr;
	
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
	char* versionString;
	
	methodString = strsep_ext(&line, " ");
	request->path = strsep_ext(&line, " ");
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

char* HTTPRequestGetPath(HTTPRequest request)
{
	return request->path;
}
