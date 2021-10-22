#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "regex.h"

#define LINELEN 1024
#define REGEX_MAX 32

#define USAGE "Usage: jgrep [OPTION]... PATTERNS [FILE]...\n"

#define IGNORE_DIR 0
#define RECURSE_DIR 1

char linebuf[LINELEN];

regex_t regexprs[REGEX_MAX];
int r = 0;

/* store given file and or directory names */
char* handles[512];
int h = 0;

typedef struct {
	char** p;
	int ln;
	int sz;
} strarr;

int num = 0;
int dirpol = IGNORE_DIR;

int isreg(char* handle) {
	struct stat s;
	memset(&s, 0, sizeof(struct stat));
	stat(handle, &s);
	return S_ISREG(s.st_mode);
}

int isdir(char* handle) {
	struct stat s;
	memset(&s, 0, sizeof(struct stat));
	stat(handle, &s);
	return S_ISDIR(s.st_mode);
}

void cleanup(regex_t regexprs[], int r, strarr* f) {
	int i = 0;
	for(i = 0; i < r; ++i)
		refree(&regexprs[i]);
	for(i = 0; i < f->ln; ++i)
		free(f->p[i]);
	if(f->p != NULL)
		free(f->p);
}

int readstdin(void) {
	while((fgets(linebuf, LINELEN, stdin)) != NULL) {
		int match = 0;

		for(int i = 0; i < r && match != 1; ++i)
			match = rematch(regexprs[r - 1], linebuf);

		if(match) {
			printf("%s", linebuf);
		}
	}
	return 1;
}

int readfiles(strarr* files) {
	char** p = files->p;
	int n = files->ln;
	int i = 0;

	while(i < n) {
		FILE* fp = fopen(p[i], "r");
		int lnum = 1;
		while((fgets(linebuf, LINELEN, fp)) != NULL) {
			int match = 0;

			for(int j = 0; j < r && match != 1; ++j)
				match = rematch(regexprs[r - 1], linebuf);

			if(match) {
				if(n > 1)
					printf("%s:", p[i]);
				if(num)
					printf("%d:", lnum);
				printf("%s", linebuf);
			}
			++lnum;
		}
		++i;
		fclose(fp);
	}
	return 1;
}

/* write me */
void recursedir(char* basepath, strarr* files) {
	int baselen = strlen(basepath);
	char* filepath = (char*)calloc((baselen + 1 + 1 + 255), sizeof(char));
	strcpy(filepath, basepath);
	filepath[baselen++] = '/';

	struct dirent* ent = NULL;
	DIR* dp = opendir(basepath);

	if(!dp) {
		free(filepath);
		closedir(dp);
		return;
	}

	while((ent = readdir(dp)) != NULL) {
		strcat(filepath, ent->d_name);

		if(strcmp(filepath + baselen, ".") == 0 || strcmp(filepath + baselen, "..") == 0) {
			filepath[baselen] = 0;
			continue;
		}
		if(isdir(filepath))
			recursedir(filepath, files);
		else if(isreg(filepath)) {
			char* fname = (char*)calloc((strlen(filepath) + 1), sizeof(char));
			strcpy(fname, filepath);

			if(files->ln == files->sz) {
				files->sz += 8;
				files->p = (char**)realloc(files->p, files->sz * sizeof(char*));
			}
			files->p[files->ln++] = fname;
		}

		filepath[baselen] = 0;
	}

	free(filepath);
	closedir(dp);
	return;
}

int main(int argc, char* argv[]) {
	if(argc == 1) {
		fprintf(stderr, "%s: too few arguments\n", argv[0]);
		return 1;
	}

	/* store final set of file names */
	strarr files = (strarr){ .p = NULL, .ln = 0, .sz = 0 };

	int c;
	while((c = getopt(argc, argv, "e:nd:f:h")) != -1) {
		switch(c) {
			case 'n':
				num = 1;
				break;
			case 'd':
				dirpol = atoi(optarg);
				break;
			case 'h':
				cleanup(regexprs, r, &files);
				fprintf(stderr, "%s", USAGE);
				return 0;
			case 'f':
				handles[h++] = optarg;
				break;
			case 'e':
				if(r == 31) {
					fprintf(stderr, "%s: too many expressions given\n", argv[0]);
					cleanup(regexprs, r, &files);
					return 1;
				}
				regexprs[r++] = recompile(optarg);
				break;
			case '?':
				cleanup(regexprs, r, &files);
				return 1;
		}
	}

	if(optind == 1)
		regexprs[r++] = recompile(argv[optind++]);

	while(optind < argc)
		handles[h++] = argv[optind++];

	switch(h) {
		case 0:
			readstdin();
			cleanup(regexprs, r, &files);
			return 0;
		default:
			files.sz = h;
			files.ln = 0;
			files.p = (char**)malloc(files.sz * sizeof(char*));

			for(int i = 0; i < h; ++i) {
				char* handle = handles[i];
				if(isreg(handle)) {
					int l = strlen(handle);
					files.p[files.ln++] = (char*)malloc((l + 1) * sizeof(char));
					strcpy(files.p[i], handle);
				}
				else if(isdir(handle))
					recursedir(handle, &files);
			}
			readfiles(&files);
			cleanup(regexprs, r, &files);
			return 0;
	}
}
