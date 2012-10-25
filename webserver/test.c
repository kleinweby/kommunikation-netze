#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"
#include "helper.h"

int main(int argc, char** argv) {
	setBlocking(0, false);
	
	WebServer server = WebServerCreate("8080");
	
	if (server)
		WebServerRunloop(server);
	
	return 0;
}
