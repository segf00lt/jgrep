#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX 8192

enum OPCODES {
	CHAR = 1,
	WILD = 2,
	JUMP = 3,
	BACK = 4,
	ALT = 5,
	CLASS = 6,
	BLINE = 7,
	ELINE = 8,
	MATCH = 11,
};

enum INSTLENS {
	CHARLEN = 2,
	WILDLEN = 1,
	JUMPLEN = 3,
	BACKLEN = 3,
	ALTLEN = 5,
	CLASSLEN = 3,
	BLINELEN = 1,
	ELINELEN = 1,
	MATCHLEN = 1,
};

enum OFFSETS {
	OP = 0,
	DATA = 1,
	CLASSBEGIN = 3,
	ARROW_0 = 1,
	ARROW_1 = 3,
};

typedef struct {
	char* bin;
	unsigned int len;
} regex_t;

char cache1[MAX];
char cache2[MAX];
regex_t cache3[MAX];
int cache4[MAX];
int cache5[MAX];

void cacheinit(void) {
	for(int i = 0; i <= MAX; ++i) {
		cache1[i] = cache2[i] = 0;
		cache3[i] = (regex_t){ .bin = NULL, .len = 0 };
		cache4[i] = cache5[i] = 0;
	}
}

/* store all compiled expressions */
char* all[MAX];
int all_pos = 0;

void reclear(void) {
	for(int i = 0; i < all_pos; ++i)
		free(all[i]);
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

/* getnum() and setnum() allow treatment of two adjacent chars as one 2 byte number */
void setnum(char* c, int n) {
	c[0] = n ^ 256;
	c[1] = n >> 8;
}

int getnum(char* c) {
	int n = 0;
	n |= c[0];
	n |= c[1] << 8;
	return n;
}

void pilefrag(regex_t frag, regex_t* stack, int* stacksize, int* stackpos) {
	if(*stacksize > 0)
		++(*stackpos);
	++(*stacksize);
	stack[*stackpos] = frag;
}
	
regex_t popfrag(regex_t* stack, int* stacksize, int* stackpos) {
	regex_t frag = stack[*stackpos];
	stack[*stackpos] = (regex_t){ .bin = NULL, .len = 0 };
	if(*stacksize > 1) {
		--(*stackpos);
		--(*stacksize);
	} else
		*stackpos = *stacksize = 0;
	return frag;
}

regex_t generate(char* post) {
	regex_t nilfrag = (regex_t){ .bin = NULL, .len = 0 };
	regex_t prog = nilfrag;
	regex_t frag0 = nilfrag;
	regex_t frag1 = nilfrag;
	regex_t frag2 = nilfrag;
	regex_t tmp = nilfrag;
	regex_t* stack = cache3;
	int stackpos = 0;
	int stacksize = 0;

	for(char* c = post; *c; ++c) {
		tmp = frag0 = frag1 = frag2 = nilfrag;
		switch(*c) {
			case '\\':
				++c;
				frag0.len = CHARLEN;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = CHAR;
				frag0.bin[DATA] = *c;
				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '^':
				frag0.len = BLINELEN;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = BLINE;
				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '$':
				frag0.len = ELINELEN;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = ELINE;
				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '|':
				frag2 = popfrag(stack, &stacksize, &stackpos);
				frag1 = popfrag(stack, &stacksize, &stackpos);

				tmp.len = JUMPLEN;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[OP] = JUMP;
				setnum(tmp.bin + ARROW_0, frag2.len + 2);

				frag1 = progcat(frag1, tmp);
				tmp = nilfrag;

				frag0.len = ALTLEN;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = ALT;
				setnum(frag0.bin + ARROW_0, 4);
				setnum(frag0.bin + ARROW_1, frag1.len + 2);

				frag0 = progcat(frag0, frag1);
				frag0 = progcat(frag0, frag2);

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '?':
				frag0 = popfrag(stack, &stacksize, &stackpos);

				tmp.len = ALTLEN;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[OP] = ALT;
				setnum(tmp.bin + ARROW_0, 4);
				setnum(tmp.bin + ARROW_1, frag0.len + 2);

				frag0 = progcat(tmp, frag0);

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '+':
				frag0 = popfrag(stack, &stacksize, &stackpos);

				tmp.len = ALTLEN;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[OP] = ALT;
				setnum(tmp.bin + ARROW_0, 4);
				setnum(tmp.bin + ARROW_1, 5);

				frag0 = progcat(frag0, tmp);

				tmp.len = BACKLEN;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[OP] = BACK;
				setnum(tmp.bin + ARROW_0, frag0.len + 1);

				frag0 = progcat(frag0, tmp);

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '*':
				frag0 = popfrag(stack, &stacksize, &stackpos);

				tmp.len = BACKLEN;
				tmp.bin = (char*)calloc(tmp.len + 1, sizeof(char));
				tmp.bin[0] = BACK;
				setnum(tmp.bin + ARROW_0, frag0.len + 6);

				frag0 = progcat(frag0, tmp);

				tmp.len = ALTLEN;
				tmp.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				tmp.bin[OP] = ALT;
				setnum(tmp.bin + ARROW_0, 4);
				setnum(tmp.bin + ARROW_1, frag0.len + 2);

				frag0 = progcat(tmp, frag0);

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '&':
				frag2 = popfrag(stack, &stacksize, &stackpos);
				frag1 = popfrag(stack, &stacksize, &stackpos);

				frag0 = progcat(frag1, frag2);

				pilefrag(frag0, stack, &stacksize, &stackpos);
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

				frag0.len = CLASSLEN + i;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = CLASS;
				setnum(frag0.bin + DATA, i);
				memcpy(frag0.bin + 3, s, i + 1);

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			case '.':
				frag0.len = WILDLEN;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = WILD;

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

			default:
				frag0.len = CHARLEN;
				frag0.bin = (char*)calloc(frag0.len + 1, sizeof(char));
				frag0.bin[OP] = CHAR;
				frag0.bin[DATA] = *c;

				pilefrag(frag0, stack, &stacksize, &stackpos);
				break;

		}
	}

	prog.len = MATCHLEN;
	prog.bin = (char*)calloc(prog.len + 1, sizeof(char));
	prog.bin[OP] = MATCH;

	for(; stacksize > 0; --stacksize)
		prog = progcat(stack[stackpos--], prog);

	return prog;
}

regex_t recompile(char* exp) {
	static int first = 1;
	cacheinit();
	char* post = parse(exp);
	printf("%s\n", post);
	regex_t re = generate(post);
	all[all_pos++] = re.bin;
	if(first)
		atexit(reclear);
	first = 0;
	return re;
}

int addthread(int* list, int len, int thread) {
	for(int i = 0; i < len; ++i) {
		if(list[i] == thread)
			return 0;
	}
	list[len] = thread;
	return 1;
}

int inclass(char* class, int len, char c) {
	for(int i = 0; i < len; ++i) {
		if(class[i] == '\\') {
			++i;
			switch(class[i]) {
				case 'z':
					if(c >= 'a' && c <= 'z')
						return 1;
					break;
				case 'Z':
					if(c >= 'A' && c <= 'Z')
						return 1;
					break;
				case '9':
					if(c >= '0' && c <= '9')
						return 1;
					break;
			}
		}
		if(class[i] == c)
			return 1;
	}
	return 0;
}

int rematch(regex_t re, char* s, int sub) {
	int* clist = cache4;
	int cl_pos = 0;
	int* nlist = cache5;
	int nl_pos = 0;

	int* tmp = NULL;

	int pc = 0;
	int sp = 0;
	int s_len = strlen(s);

	clist[cl_pos++] = pc;
	for(; sp <= s_len; ++sp) {
		for(int i = 0; i < cl_pos; ++i) {
			pc = clist[i];
			char op = re.bin[pc];
			int a0 = pc + ARROW_0;
			int a1 = pc + ARROW_1;
			int cl = 0;

			switch(op) {
				case CHAR:
					pc += DATA;
					if(re.bin[pc] == s[sp])
						addthread(nlist, nl_pos++, pc + ARROW_0);
					break;

				case WILD:
					addthread(nlist, nl_pos++, a0);
					break;

				case JUMP:
					addthread(clist, cl_pos++, a0 + getnum(re.bin + a0));
					break;

				case BACK:
					addthread(clist, cl_pos++, a0 - getnum(re.bin + a0));
					break;

				case ALT:
					addthread(clist, cl_pos++, a0 + getnum(re.bin + a0));
					addthread(clist, cl_pos++, a1 + getnum(re.bin + a1));
					break;

				case CLASS:
					cl = getnum(re.bin + pc + DATA);
					if(inclass(re.bin + pc + CLASSBEGIN, cl, s[sp]))
						addthread(nlist, nl_pos++, pc + CLASSLEN + cl);
					break;

				case BLINE:
					if(sp == 0)
						addthread(clist, cl_pos++, a0);
					break;

				case ELINE:
					if(sp == (s_len - 1))
						addthread(clist, cl_pos++, a0);
					break;

				case MATCH:
					if(sp == s_len)
						return 1;
					break;
			}
		}
		tmp = clist;
		clist = nlist;
		nlist = tmp;
		cl_pos = nl_pos;
		nl_pos = 0;
	}

	return 0;
}

int main(int argc, char* argv[]) {
	regex_t re = recompile(argv[1]);
	for(int i = 0; i < re.len; ++i)
		printf("\\0x%x", re.bin[i]);
	printf("\n");
	printf("%d\n", rematch(re, argv[2], 1));
	return 0;
}
