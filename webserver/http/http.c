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

#include "http.h"

#include <stdlib.h>

const char* kHTTPLineDelimiter = "\r\n";
const char* kHTTPContentDelimiter = "\r\n\r\n";
const char* kHTTPHeaderDelimiter = ":";

char* HTTPStatusNameFromCode(HTTPStatusCode code)
{
	switch (code) {
	case kHTTPContinue:
		return "Continue";
	case kHTTPSwitchingProtocols:
		return "Switching Protocols";
	
	case kHTTPOK:
		return "OK";
	case kHTTPCreated:
		return "Created";
	case kHTTPAccepted:
		return "Accepted";
	case kHTTPNonAuthoritativeInformation:
		return "Non Authoritative Information";
	case kHTTPNoContent:
		return "No Content";
	case kHTTPResetContent:
		return "Reset Content";
	case kHTTPPartialContent:
		return "Partial Content";
	
	case kHTTPMultipleChoices:
		return "Multiple Choices";
	case kHTTPMovedPermanently:
		return "Moved Permanently";
	case kHTTPFound:
		return "Found";
	case kHTTPSeeOther:
		return "See Other";
	case kHTTPNotModified:
		return "Not Modified";
	case kHTTPUseProxy:
		return "Use Proxy";
	case kHTTPTemporaryRedirect:
		return "Temporary Redirect";
	
	case kHTTPBadRequest:
		return "Bad Request";
	case kHTTPBadUnauthorized:
		return "Unauthorized";
	case kHTTPBadPaymentRequired:
		return "Payment Required";
	case kHTTPBadForbidden:
		return "Forbidden";
	case kHTTPBadNotFound:
		return "Not Found";
	case kHTTPBadMethodNotAllowed:
		return "Method Not Allowed";
	case kHTTPBadNotAcceptable:
		return "Not Acceptable";
	case kHTTPBadProxyAuthenticationRequired:
		return "Proxy Authentication Required";
	case kHTTPBadRequestTimeout:
		return "Request Timeout";
	case kHTTPBadConflict:
		return "Conflict";
	case kHTTPBadGone:
		return "Gone";
	case kHTTPBadLengthRequired:
		return "Length Required";
	case kHTTPBadPreconditionFailed:
		return "Precondition Failed";
	case kHTTPBadRequestEntityTooLarge:
		return "Request Entity Too Large";
	case kHTTPBadRequestURITooLong:
		return "Request URI Too Long";
	case kHTTPBadUnsupportedMediaType:
		return "Unsupported Media Type";
	case kHTTPBadRequestedRangeNotSatisfiable:
		return "Requested Range Not Satisfiable";
	case kHTTPBadExpectationFailed:
		return "Expectation Failed";
	
	case kHTTPErrorNotImplemented:
		return "Not Implemented";
	case kHTTPErrorBadGateway:
		return "Bad Gateway";
	case kHTTPErrorServiceUnavailable:
		return "Service Unavailable";
	case kHTTPErrorGatewayTimeout:
		return "Gateway Timeout";
	case kHTTPErrorVersionNotSupported:
		return "Version Not Supported";
	}
	
	return NULL;
}
