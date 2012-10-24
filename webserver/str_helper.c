#include "str_helper.h"

#include <string.h>
#include <ctype.h>

char* strsep_ext(char** stringp, const char* delim) {
	char* value;
	
	do {
		value = strsep(stringp, delim);
	} while (value && strlen(value) == 0);
		
	return value;
}

char* strtrim(char* string) {
	while(isspace(*string)) 
		string++;
	
	while(strlen(string) > 0 && isspace(string[strlen(string)]))
		string[strlen(string)] = '\0';
	
	return string;
}