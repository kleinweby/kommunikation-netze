#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"

char *requestTemplate = "GET / HTTP/1.1\r\n"
	"Host: alfredo.spei.ch:8888\r\n"
	"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.26.14 (KHTML, like Gecko) Version/6.0.1 Safari/536.26.14\r\n"
	"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
	"DNT: 1\r\n"
	"Accept-Language: en-us\r\n"
	"Accept-Encoding: gzip, deflate\r\n"
	"Connection: keep-alive\r\n\r\n";

int main(int argc, char** argv) {
	WebServer server = WebServerCreate("8080");
	
	WebServerRunloop(server);
	
	return 0;
}
