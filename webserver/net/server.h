
#ifndef _SERVER_H_
#define _SERVER_H_

#include "utils/DispatchQueue.h"
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
