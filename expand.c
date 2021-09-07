#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helper.h"

/*
 * crange: Generate char* in range(f, t)
 * @param f Lowerbound
 * @param t Upperbound
 * @return s String generated, or NULL if f == t
 */
char* crange(char f, char t) {
	int l = t - f;
	if(l == 0) return NULL;

	char* s = (char*)calloc(abs(l) + 2, sizeof(char));
	char* pos = s;

	if(l > 0)
		for(char c = f++; c <= t;*(pos++) = c++);
	else
		for(char c = f--; c >= t;*(pos++) = c--);

	return s;
}

char* expand(char* str) {
}

int main(int argc, char* argv[]) {
	char* s = crange(*argv[1], *argv[2]);
	printf("%s\n", s);
	free(s);

	return 0;
}
