#ifndef REGEX_H
#define REGEX_H

#define MAX 4096

typedef struct {
	char* bin;
	unsigned int len;
} regex_t;

regex_t recompile(char* exp);
int rematch(regex_t re, char* s, int sub);

#endif
