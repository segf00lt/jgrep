#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX 4096

enum OPCODES {
	CHAR = 1,
	WILD = 2,
	JUMP = 3,
	BACK = 4,
	ALT = 5,
	CLASS = 6,
	BLINE = 130,
	ELINE = 140,
	MATCH = 11,
};

typedef struct {
	char* bin;
	unsigned int len;
} regex_t;

char cache1[MAX];
char cache2[MAX];
regex_t cache3[MAX];

void cacheinit(void) {
	for(int i = 0; i <= MAX; ++i) {
		cache1[i] = cache2[i] = 0;
		cache3[i] = (regex_t){ .bin = NULL, .len = 0 };
	}
}

char* parse(char* exp) {
	int l_exp = strlen(exp);
	if(l_exp == 0) {
		fprintf(stderr, "regex: parse: empty expression\n");
		exit(1);
	}
	if(l_exp >= MAX) {
		fprintf(stderr, "regex: parse: expression too large\n");
		exit(1);
	}
	char* stack = cache1;
	int stackpos = 0;
	int stacksize = 0;

	char* buf = cache2;
	int i = 0;

	int n_atoms = 0;
	int prev_atoms[MAX];
	int j = 0;

	char baseop = '&';

	char pc = 0;
	char nc = 0;

	for(char* c = exp; *c; ++c) {
		switch(*c) {
			case '(':
				if(baseop != '&') {
					fprintf(stderr, "regex: parse: group operator within class\n");
					exit(1);
				}
				if(n_atoms > 1) {
					buf[i++] = baseop;
					--n_atoms;
				}

				prev_atoms[j++] = n_atoms;

				n_atoms = 0;

				if(stacksize > 0)
					++stackpos;
				else
					stackpos = 0;

				stack[stackpos] = *c;
				++stacksize;
				break;

			case '|':
				if(baseop != '&') {
					fprintf(stderr, "regex: parse: alternation within class\n");
					exit(1);
				}
				if(n_atoms == 0) {
					fprintf(stderr, "regex: parse: lone or incomplete alternation");
					exit(1);
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = baseop;

				for(; stacksize > 0 && stackpos >= 0 && stack[stackpos] != '('; --stacksize)
				{
					buf[i++] = stack[stackpos];
					stack[stackpos--] = 0;
				}

				if(stacksize > 0)
					++stackpos;
				else
					stackpos = 0;

				stack[stackpos] = *c;
				++stacksize;
				break;

			case ')':
				if(baseop != '&') {
					fprintf(stderr, "regex: parse: group operator within class\n");
					exit(1);
				}
				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "regex: parse: mismatched or empty parentheses\n");
					exit(1);
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = baseop;

				for(; stacksize > 0 && stackpos >= 0 && stack[stackpos] != '('; --stacksize)
				{
					buf[i++] = stack[stackpos];
					stack[stackpos--] = 0;
				}

				if(stacksize == 0) {
					stackpos = 0;
					stack[stackpos] = 0;
				} else {
					stack[stackpos--] = 0;
					--stacksize;
				}

				n_atoms = prev_atoms[--j] + 1;
				break;

			case '[':
				if(baseop != '&') {
					fprintf(stderr, "regex: parse: mismatched or nested square brackets\n");
					exit(1);
				}

				if(n_atoms > 1) {
					buf[i++] = baseop;
					--n_atoms;
				}

				buf[i++] = *c;
				prev_atoms[j++] = n_atoms;
				n_atoms = 0;
				baseop = 127;
				break;

			case '-':
				if(baseop == '&') {
					if(n_atoms > 1) {
						buf[i++] = baseop;
						--n_atoms;
					}
					buf[i++] = *c;
					++n_atoms;
					break;
				} else if(n_atoms == 0) {
					fprintf(stderr, "regex: parse: invalid range\n");
					exit(1);
				}

				pc = *(c - 1);
				nc = *(c + 1);

				if(nc <= pc || pc == 127 || pc <= 31) {
					fprintf(stderr, "regex: parse: invalid range\n");
					exit(1);
				}

				if(pc == 'a' && nc == 'z' || pc == 'A' && nc == 'Z' || pc == '0' && nc == '9') {
					buf[i - 1] = '\\';
					buf[i++] = nc;
				} else {
					char* tmp = (char*)calloc((nc - pc) + 2, sizeof(char));
					char* s = tmp;
					for(char a = pc++; a <= nc; *(s++) = a++);
					--i;
					for(char* pos = tmp; *pos != 0; ++pos) {
						if(*pos >= 91 && *pos <= 93)
							buf[i++] = '\\';
						buf[i++] = *pos;
					}
					free(tmp);
				}
				++n_atoms;
				++c;

				break;

			case ']':
				if(baseop == '&' || n_atoms == 0) {
					fprintf(stderr, "regex: parse: mismatched or empty square brackets\n");
					exit(1);
				}
				buf[i++] = *c;
				n_atoms = prev_atoms[--j] + 1;
				baseop = '&';
				break;

			case '?':
			case '+':
			case '*':
				if(n_atoms == 0) {
					fprintf(stderr, "regex: parse: lone repetition operator\n");
					exit(1);
				}
				buf[i++] = *c;
				break;

			case '^':
			case '$':
				if(n_atoms > 1) {
					--n_atoms;
					buf[i++] = baseop;
				}

				buf[i++] = *c;
				break;

			case '\\':
				if(n_atoms > 1) {
					--n_atoms;
					buf[i++] = baseop;
				}

				nc = *(c + 1);
				if(nc == 0) {
					fprintf(stderr, "regex: parse: lone escape operator\n");
					exit(1);
				}
				switch(nc) {
					case '.':
					case '(':
					case '|':
					case ')':
					case '[':
					case '-':
					case ']':
					case '?':
					case '+':
					case '*':
					case '&':
					case '^':
					case '$':
					case '\\':
						buf[i++] = *c;
				}

				++c;
				buf[i++] = *c;
				++n_atoms;
				break;

			default:
				if(n_atoms > 1) {
					--n_atoms;
					buf[i++] = baseop;
				}

				buf[i++] = *c;
				++n_atoms;
				break;
		}
	}
	if(baseop != '&') {
		fprintf(stderr, "regex: parse: mismatched square brackets\n");
		exit(1);
	}

	if(j != 0) {
		fprintf(stderr, "regex: parse: mismatched parentheses\n");
		exit(1);
	}

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	for(; stacksize > 0; --stacksize) {
		buf[i++] = stack[stackpos];
		stack[stackpos--] = 0;
	}

	return buf;
}

regex_t progcat(regex_t f1, regex_t f2) {
	regex_t f0;
	f0.len = f1.len + f2.len;
	f0.bin = (char*)calloc(f0.len + 1, sizeof(char));

	int i = 0;
	int j = 0;
	for(; i < f1.len; ++i)
		f0.bin[i] = f1.bin[i];
	for(int i = f1.len; i < f0.len; ++i)
		f0.bin[i] = f2.bin[j++];

	free(f1.bin);
	free(f2.bin);
	return f0;
}

regex_t generate(char* post) {
	regex_t nilfrag = (regex_t){ .bin = NULL, .len = 0 };
	regex_t prog = nilfrag;
	regex_t frag0 = nilfrag;
	regex_t frag1 = nilfrag;
	regex_t frag2 = nilfrag;
	regex_t tmp = nilfrag;
	char c1 = 0;
	char c2 = 0;
	regex_t* stack = cache3;
	int stackpos = 0;
	int stacksize = 0;
	
	inline void pile(regex_t frag) {
		if(stacksize > 0)
			++stackpos;
		++stacksize;
		stack[stackpos] = frag;
	}
	
	inline regex_t pop(void) {
		regex_t frag = stack[stackpos];
		stack[stackpos] = nilfrag;
		if(stacksize > 1) {
			--stackpos;
			--stacksize;
		} else
			stackpos = stacksize = 0;
		return frag;
	}

	for(char* c = post; *c; ++c) {
		tmp = frag0 = frag1 = frag2 = nilfrag;
		switch(*c) {
			case '\\':
				++c;
				frag0.len = 2;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = CHAR;
				frag0.bin[1] = *c;
				pile(frag0);
				break;

			case '^':
				frag0.len = 1;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = BLINE;
				pile(frag0);
				break;

			case '$':
				frag0.len = 1;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = ELINE;
				pile(frag0);
				break;

			case '|':
				frag2 = pop();
				frag1 = pop();

				tmp.len = 3;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[0] = JUMP;
				c1 = (frag2.len + 2) ^ 256;
				c2 = (frag2.len + 2) >> 8;
				tmp.bin[1] = c1;
				tmp.bin[2] = c2;
				c1 = c2 = 0;

				frag1 = progcat(frag1, tmp);
				tmp = nilfrag;

				frag0.len = 5;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = ALT;
				frag0.bin[1] = 4;
				frag0.bin[2] = 0;
				c1 = (frag1.len + 2) ^ 256;
				c2 = (frag1.len + 2) >> 8;
				frag0.bin[3] = c1;
				frag0.bin[4] = c2;
				c1 = c2 = 0;

				frag0 = progcat(frag0, frag1);
				frag0 = progcat(frag0, frag2);

				pile(frag0);
				break;

			case '?':
				frag0 = pop();

				tmp.len = 5;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[0] = ALT;
				tmp.bin[1] = 4;
				tmp.bin[2] = 0;
				c1 = (frag0.len + 2) ^ 256;
				c2 = (frag0.len + 2) >> 8;
				tmp.bin[3] = c1;
				tmp.bin[4] = c2;

				frag0 = progcat(tmp, frag0);

				pile(frag0);
				break;

			case '+':
				frag0 = pop();

				/* "\0x05\0x04\0x00\0x05\0x00" */
				tmp.len = 5;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[0] = ALT;
				tmp.bin[1] = 4;
				tmp.bin[2] = 0;
				tmp.bin[3] = 5;
				tmp.bin[4] = 0;

				frag0 = progcat(frag0, tmp);
				printf("%i\n", frag0.len);

				/* "\0x04<length of frag0 + 6>\0x00" */
				tmp.len = 3;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[0] = BACK;
				c1 = (frag0.len + 1) ^ 256;
				c2 = (frag0.len + 1) >> 8;
				tmp.bin[1] = c1;
				tmp.bin[2] = c2;

				frag0 = progcat(frag0, tmp);

				pile(frag0);
				break;

			case '*':
				frag0 = pop();

				/* "\0x04<length of frag0 + 1 + 5>\0x00" */
				tmp.len = 3;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[0] = BACK;
				c1 = (frag0.len + 6) ^ 256;
				c2 = (frag0.len + 6) >> 8;
				tmp.bin[1] = c1;
				tmp.bin[2] = c2;

				frag0 = progcat(frag0, tmp);

				/* "\0x05\0x02\0x00<length of frag0 + 2>" */
				tmp.len = 5;
				tmp.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				tmp.bin[0] = ALT;
				tmp.bin[1] = 4;
				tmp.bin[2] = 0;
				c1 = (frag0.len + 2) ^ 256;
				c2 = (frag0.len + 2) >> 8;
				tmp.bin[3] = c1;
				tmp.bin[4] = c2;
				c1 = c2 = 0;

				frag0 = progcat(tmp, frag0);

				pile(frag0);
				break;

			case '&':
				frag2 = pop();
				frag1 = pop();

				frag0 = progcat(frag1, frag2);

				pile(frag0);
				break;

			case '[':
				char s[97];
				int i = 0;
				for(++c; *c != ']'; ++c) {
					if(*c == 127)
						continue;
					if(*c == '\\') {
						s[i++] = *c;
						++c;
					}
					s[i++] = *c;
				}
				s[i] = 0;

				frag0.len = 3 + i;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = CLASS;
				c1 = i ^ 256;
				c2 = i >> 8;
				frag0.bin[1] = c1;
				frag0.bin[2] = c2;
				memcpy(frag0.bin + 3, s, i + 1);

				pile(frag0);
				break;

			case '.':
				frag0.len = 1;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = WILD;

				pile(frag0);
				break;

			default:
				frag0.len = 2;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[0] = CHAR;
				frag0.bin[1] = *c;

				pile(frag0);
				break;

		}
	}

	prog.len = 1;
	prog.bin = (char*)calloc(prog.len + 1, sizeof(char));
	prog.bin[0] = MATCH;

	for(; stacksize > 0; --stacksize)
		prog = progcat(stack[stackpos--], prog);

	return prog;
}

int main(int argc, char* argv[]) {
	cacheinit();
	char* post = parse(argv[1]);
	printf("%s\n", post);
	regex_t re = generate(post);
	for(int i = 0; i < re.len; ++i)
		printf("\\0x%x", re.bin[i]);
	printf("\n");
	free(re.bin);
	return 0;
}
