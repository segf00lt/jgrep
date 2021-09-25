#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"

#define LINELEN 1024

int main(int argc, char* argv[]) {
	FILE* fp = NULL;
	char buf[LINELEN];

	if(argc == 2)
		fp = stdin;
	else
		fp = fopen(argv[2], "r");

	regex_t regexp = recomp(argv[1]);

	while((fgets(buf, LINELEN, fp)) != NULL) {
		if(rematchsub(regexp, buf))
			printf("%s", buf);
	}

	fclose(fp);

	return 0;
}
