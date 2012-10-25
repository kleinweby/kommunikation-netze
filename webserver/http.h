#include "server.h"

#include <stdbool.h>
#include <netinet/in.h>

typedef struct _HTTPRequest* HTTPRequest; 
typedef struct _HTTPConnection* HTTPConnection;
typedef struct _HTTPResponse* HTTPResponse;

typedef enum { 
	kHTTPMethodGet,
	kHTTPMethodUnkown
} HTTPMethod;

typedef enum {
	kHTTPVersion_1_0,
	kHTTPVersion_1_1,
	kHTTPVersionUnkown
} HTTPVersion;

typedef enum {
	kHTTPContinue = 100,
	kHTTPSwitchingProtocols = 101,
	
	kHTTPOK = 200,
	kHTTPCreated = 201,
	kHTTPAccepted = 202,
	kHTTPNonAuthoritativeInformation = 203,
	kHTTPNoContent = 204,
	kHTTPResetContent = 205,
	kHTTPPartialContent = 206,
	
	kHTTPMultipleChoices = 300,
	kHTTPMovedPermanently = 301,
	kHTTPFound = 302,
	kHTTPSeeOther = 303,
	kHTTPNotModified = 304,
	kHTTPUseProxy = 305,
	kHTTPTemporaryRedirect = 307,
	
	kHTTPBadRequest = 400,
	kHTTPBadUnauthorized = 401,
	kHTTPBadPaymentRequired = 402,
	kHTTPBadForbidden = 403,
	kHTTPBadNotFound = 404,
	kHTTPBadMethodNotAllowed = 405,
	kHTTPBadNotAcceptable = 406,
	kHTTPBadProxyAuthenticationRequired = 407,
	kHTTPBadRequestTimeout = 408,
	kHTTPBadConflict = 409,
	kHTTPBadGone = 410,
	kHTTPBadLengthRequired = 411,
	kHTTPBadPreconditionFailed = 412,
	kHTTPBadRequestEntityTooLarge = 413,
	kHTTPBadRequestURITooLong = 414,
	kHTTPBadUnsupportedMediaType = 415,
	kHTTPBadRequestedRangeNotSatisfiable = 416,
	kHTTPBadExpectationFailed = 417,
	
	kHTTPErrorNotImplemented = 501,
	kHTTPErrorBadGateway = 502,
	kHTTPErrorServiceUnavailable = 503,
	kHTTPErrorGatewayTimeout = 504,
	kHTTPErrorVersionNotSupported = 505
} HTTPStatusCode;

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
// Returns the value saved for key or NULL it key does not exists
//
const char* HTTPRequestGetHeaderValueForKey(HTTPRequest request, const char* key);

//
// Destroys an http request and releases all resources
//
void HTTPRequestDestroy(HTTPRequest request);

//
// Creates and accepts a new http connection
//
HTTPConnection HTTPConnectionCreate(Server server);

//
// Destroys the connection
//
void HTTPConnectionDestroy(HTTPConnection connection);

//
// Creates a new http response associated with an
// given connection
//
HTTPResponse HTTPResponseCreate(HTTPConnection connection);

//
// Destroys a http response
//
void HTTPResponseDestroy(HTTPResponse response);

//
// Set a given status code
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
// This also sets the length header if fd supports it and closes
// the fd upon completion
//
void HTTPResponseSetResponseFileDescriptor(HTTPResponse response, int fd);

//
// Sends the response over the connection.
//
// Warning: this blocks.
//
void HTTPResponseSendComplete(HTTPResponse response);