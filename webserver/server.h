
#ifndef _SERVER_H_
#define _SERVER_H_

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
//
//
typedef void(*WebServerPollCallback)(int revents, void* userInfo);
void WebServerRegisterPollForSocket(WebServer webServer, int socket, int events, WebServerPollCallback callback, void* userInfo);
void WebServerUnregisterPollForSocket(WebServer webServer, int socket);

//
// Return the server socket
//
int ServerGetSocket(Server server);

WebServer ServerGetWebServer(Server server);

#endif /* _SERVER_H_ */
