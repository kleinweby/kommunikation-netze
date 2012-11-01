#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include "http.h"
#include "httpconnection.h"

//
// Creates a new http response associated with an
// given connection
//
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
// Warning: this blocks.
//
void HTTPResponseSendComplete(HTTPResponse response);

//
// Marks the response as finished. It will be transmitted as soon
// as possible. Changing the response after calling this
// will result in an error.
//
void HTTPResponseFinish(HTTPResponse response);

#endif /* _HTTPRESPONSE_H_ */
