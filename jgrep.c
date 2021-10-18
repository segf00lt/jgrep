#include <stdio.h>
#include <sttblib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "regex.h"

#define LINELEN 1024

#define USAGE "Usage: jgrep [OPTION]... PATTERNS [FILE]...\n"

#define IGNORE_DIR 0
#define RECURSE_DIR 1

regex_t REGEXPS[32];

struct Filelist {
	FILE** p;
	int sz;
};

struct Relist {
	regex_t* p;
	int sz;
};

struct tnode {
	char* name;
	int fd;
};

struct Table {
	struct tnode* p;
	int sz;
};

void cleanup(Filelist* fl, Table* tbl, Relist* rl) {
	for(int i = 0; i < fl->sz; ++i)
		fclose(fl->p[i]);
	free(fl->p);

	free(tbl->p);

	for(int i = 0; i < rl->sz; ++i)
		refree(rl->p[i]);
	free(rl->p);
}

void recursedir(DIR* dp, Filelist* f) {
/* write me */
}

int isreg(int fd) {
	struct stat* s;
	stat(fd, s);
	return S_ISREG(s->st_mode);
}

int isdir(int fd) {
	struct stat* s;
	fstat(fd, s);
	return S_ISDIR(s->st_mode);
}

int dirfirst(const void* a, const void* b) {
	struct tnode tn_a = *((struct tnode*)a);
	struct tnode tn_b = *((struct tnode*)b);

	return isreg(tn_a.fd) && isdir(tn_b.fd);
}

int main(int argc, char* argv[]) {
	char linebuf[LINELEN];

	struct Table tbl = (struct Table){ .p = (struct tnode*)malloc(8 * sizeof(struct tnode)), .sz = 8 };
	int tbl_pos = 0;

	struct tnode tn;

	struct Filelist fl;
	int fl_pos = 0;

	struct Relist rl = (struct Relist){ .p = REGEXPS };
	int rl_pos = 0;

	int NUMBER = 0;
	int DIRPOLICY = IGNORE_DIR;

	while((c = getopt(argc, argv, "e:nd:f:h")) != -1) {
		switch(c) {
			case 'f':
				if(tbl_pos + 1 == tbl.sz - (tbl.sz / 2)) {
					tbl.sz *= 4;
					tbl.p = (struct tnode*)realloc(tbl.p, tbl.sz * sizeof(struct tnode));
				}
				tn.name = strdup(optarg);
				tn.fd = open(optarg, O_RDONLY);
				tbl.p[tbl_pos++] = tn;
				break;
			case 'e':
				if(rl_pos == 31) {
					fprinf(stderr, "jgrep: too many expressions given\n");
					cleanup(&fl, &tbl, &rl);
					return 1;
				}
				rl.p[rl_pos++] = recompile(optarg);
				break;
			case 'n':
				NUMBER = 1;
				break;
			case 'd':
				DIRPOLICY = atoi(optarg);
				break;
			case 'h':
				fprintf(stderr, "%s", USAGE);
				cleanup(&fl, &tbl, &rl);
				return 1;
			case '?':
				cleanup(&fl, &tbl, &rl);
				return 1;
		}
	}

	if(optind == 1) {
		rl.p[rl_pos++] = recompile(argv[optind++]);
	}

	for(; optind < argc; ++optind) {
		char* arg = argv[optind];
		if(tbl_pos + 1 == tbl.sz - (tbl.sz / 2)) {
			tbl.sz *= 4;
			tbl.p = (struct tnode*)realloc(tbl.p, tbl.sz * sizeof(struct tnode));
		}
		tn.name = strdup(arg);
		tn.fd = open(arg, O_RDONLY);
		tbl.p[tbl_pos++] = tn;
	}

	fl.sz = tbl.sz;
	fl.p = (FILE**)malloc(fl.sz * (FILE*));

	switch(tbl_pos) {
		case 0:
			fl.p[0] = stdin;
			fl.sz = 1;
			break;
		default:
			qsort((void*)tbl.p, tbl.sz, sizeof(struct tnode), dirfirst);

			for(tbl_pos = 0; tbl_pos < tbl.sz; ++tbl_pos) {
				tn = tbl.p[tbl_pos];
				DIR* dp = NULL;

				if(isdir(tn.fd)) {
					if(DIRPOLICY == IGNORE_DIR)
						continue;

					dp = fdopendir(tn.fd);
					recursedir(dp, &fl);
				}
				else if(isreg(tn.fd))
					fl.p[fl_pos++] = fdopen(tn.fd, "r");
				else
					fprintf(stderr, "%s: binary file matched\n", tn.name);
			}
			break;
	}

	for(fl_pos = 0; fl_pos < fl.sz; ++fl_pos) {
		FILE* fp = fl.p[fl_pos];
		while((fgets(linebuf, LINELEN, fp)) != NULL) {
			int match = 0;
			int lnum = 0;

			for(rl_pos = 0; rl_pos < rl.sz && match != 1; ++rl_pos)
				match = rematch(rl.p[rl_pos], linebuf);

			if(match) {
				if(NUMBER)
					printf("%d: ", lnum++);
				printf("%s", linebuf);
			}
		}
	}

	cleanup(&fl, &tbl, &rl);

	return 0;
}
