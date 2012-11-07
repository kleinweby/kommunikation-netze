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

#ifndef _HTTP_H_
#define _HTTP_H_

#include "utils/object.h"

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

DECLARE_CLASS(HTTPResponse);
DECLARE_CLASS(HTTPRequest); 
DECLARE_CLASS(HTTPConnection);

#endif /* _HTTP_H_ */
