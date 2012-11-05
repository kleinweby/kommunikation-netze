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
