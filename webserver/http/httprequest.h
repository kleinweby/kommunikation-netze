#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "http/http.h"

#include <stdbool.h>

//
// Returns true when the buffer points to a
// location which could be a http request
// (means there is the magic \r\n\r\n included)
//
bool HTTPCanParseBuffer(char* buffer);

//
// Creates an http request with a given buffer.
//
HTTPRequest HTTPRequestCreate(char* buffer);

//
// Returns the method specified in the request
//
HTTPMethod HTTPRequestGetMethod(HTTPRequest request);

//
// Returns the version specfied in the request
//
HTTPVersion HTTPRequestGetVersion(HTTPRequest request);

//
// Returns the path specified in the request
//
char* HTTPRequestGetPath(HTTPRequest request);

//
// Returns the value saved for key or NULL if key does not exists
//
const char* HTTPRequestGetHeaderValueForKey(HTTPRequest request, const char* key);

#endif /* _HTTPREQUEST_H_ */
