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

#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "http.h"
#include "httpconnection.h"

//
// Creates a new http response associated with an
// given connection
//
OBJECT_RETURNS_RETAINED
HTTPResponse HTTPResponseCreate(HTTPConnection connection);

//
// Set a given status code
//
// If the code is an bad or error code, the response
// will also be configured to display a informal document
// in that case. If you want to customize this, reset the response
// after this call.
//
void HTTPResponseSetStatusCode(HTTPResponse response, HTTPStatusCode code);

//
// Set a given value for the header field.
//
void HTTPResponseSetHeaderValue(HTTPResponse response, const char* key, const char* value);

//
// Set a string to be delivered as response.
//
// This also sets the Length header and frees string upon completion.
//
void HTTPResponseSetResponseString(HTTPResponse response, char* string);

//
// Set a file descriptor to be delivered as response
//
// This closes the fd upon completion
//
void HTTPResponseSetResponseFileDescriptor(HTTPResponse response, int fd);

//
// Sends the response over the connection.
//
//
bool HTTPResponseSend(HTTPResponse response);

//
// Marks the response as finished. It will be transmitted as soon
// as possible. Changing the response after calling this
// will result in an error.
//
void HTTPResponseFinish(HTTPResponse response);

#endif /* _HTTPRESPONSE_H_ */
