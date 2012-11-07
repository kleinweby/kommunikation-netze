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

#ifndef _HTTPCONNECTION_H_
#define _HTTPCONNECTION_H_

#include "net/server.h"
#include "http/http.h"
#include "http/httprequest.h"
#include "http/httpresponse.h"

#include <stdbool.h>
#include <netinet/in.h>

//
// Creates and accepts a new http connection
//
HTTPConnection HTTPConnectionCreate(Server server, int socket, struct sockaddr_in6 info);

//
// Closes the connection
//
void HTTPConnectionClose(HTTPConnection connection);

//
// Get the request associated with the connection, if any
//
HTTPRequest HTTPConnectionGetRequest(HTTPConnection connection);

//
// Get the response associated with the connection, if any
//
HTTPResponse HTTPConnectionGetResponse(HTTPConnection connection);

//
// Sends data over the connection. Works similar to send (2)
// but already handles most of the errors (and probbaly closes
// the connection in return)
//
ssize_t HTTPConnectionSend(HTTPConnection connection, const void *buffer, size_t length);

//
// Similar to HTTPConnectionSend but used an fd to send the
// data.
//
// Returns true if everything is sent, false if should be recalled
// when data to the connection can be written
//
bool HTTPConnectionSendFD(HTTPConnection connection, int fd, off_t* offset, size_t length);

#endif /* _HTTPCONNECTION_H_ */
