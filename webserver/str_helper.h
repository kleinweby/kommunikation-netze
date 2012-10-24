//
// When strsep would return an empty string, this
// function automaticly proceeds to the next emtpy one
//
char* strsep_ext(char** stringp, const char* delim);

//
// Trims the whitespace from the beginning and the end.
// (Modifies the original string)
//
char* strtrim(char* string);