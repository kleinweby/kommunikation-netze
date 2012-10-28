#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "net/server.h"
#include "utils/helper.h"

int main(int argc, char** argv) {
#pragma unused(argc)
#pragma unused(argv)
	
	setBlocking(0, false);
	
	WebServer server = WebServerCreate("8080");
	
	if (server)
		WebServerRunloop(server);
	
	return 0;
}
