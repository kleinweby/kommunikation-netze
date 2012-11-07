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

#ifndef _SERVER_H_
#define _SERVER_H_

#include "utils/object.h"
#include "utils/dispatchqueue.h"
#include "net/Poll.h"

typedef struct _WebServer* WebServer;
typedef struct _Server* Server;

//
// Initialize the web server with the cmd arguments
//
WebServer WebServerCreate(char* port);

//
// Start handling request
//
void WebServerRunloop(WebServer server);

//
// Return the server socket
//
int ServerGetSocket(Server server);

//
// Return the queue that should be used
// for input reading from the client
//
DispatchQueue ServerGetInputDispatchQueue(Server server);

//
// Return the queue that should be used
// for output writing from the client
//
DispatchQueue ServerGetOutputDispatchQueue(Server server);

//
// Return the queue that should be used
// for processing
//
DispatchQueue ServerGetProcessingDispatchQueue(Server server);

//
// Return the poll that should be used
//
Poll ServerGetPoll(Server server);

//
// Get the greater webserver of a specifc server
//
WebServer ServerGetWebServer(Server server);

#endif /* _SERVER_H_ */
