#ifndef _HTTP_H_
#define _HTTP_H_

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

extern const char* kHTTPLineDelimiter;
extern const char* kHTTPContentDelimiter;
extern const char* kHTTPHeaderDelimiter;

typedef struct _HTTPResponse* HTTPResponse;
typedef struct _HTTPRequest* HTTPRequest; 
typedef struct _HTTPConnection* HTTPConnection;

#endif /* _HTTP_H_ */
