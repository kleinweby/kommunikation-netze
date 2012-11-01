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
HTTPConnection HTTPConnectionCreate(Server server, int socket, struct sockaddr info);

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

ssize_t HTTPConnectionSendFD(HTTPConnection connection, int fd, size_t length);

#endif /* _HTTPCONNECTION_H_ */
