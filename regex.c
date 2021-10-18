#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"

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

/* cache for parse() and generate() */
static char cache1[MAX];
static char cache2[MAX];
static regex_t cache3[MAX];

static void cacheinit(void) {
	for(int i = 0; i <= MAX; ++i) {
		cache1[i] = cache2[i] = 0;
		cache3[i] = (regex_t){ .bin = NULL, .len = 0 };
	}
}

static char* parse(char* exp) {
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

/* getnum() and setnum() allow treatment of two adjacent chars as one 2 byte number */
static inline void setnum(char* c, int n) {
	c[0] = n ^ 256;
	c[1] = n >> 8;
}

static inline int getnum(char* c) {
	int n = 0;
	n |= c[0];
	n |= c[1] << 8;
	return n;
}

static void pilefrag(regex_t frag, regex_t* stack, int* stacksize, int* stackpos) {
	if(*stacksize > 0)
		++(*stackpos);
	++(*stacksize);
	stack[*stackpos] = frag;
}
	
static regex_t popfrag(regex_t* stack, int* stacksize, int* stackpos) {
	regex_t frag = stack[*stackpos];
	stack[*stackpos] = (regex_t){ .bin = NULL, .len = 0 };
	if(*stacksize > 1) {
		--(*stackpos);
		--(*stacksize);
	} else
		*stackpos = *stacksize = 0;
	return frag;
}

static regex_t progcat(regex_t f1, regex_t f2) {
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

static regex_t generate(char* post) {
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
	cacheinit();
	char* post = parse(exp);
	regex_t re = generate(post);
	return re;
}

void refree(regex_t* re) {
	free(re->bin);
}

typedef struct {
	int* t;
	int pos;
} Threadlist;

static int addthread(Threadlist* list, int thread) {
	for(int i = 0; i < list->pos; ++i) {
		if(list->t[i] == thread)
			return 0;
	}
	list->t[list->pos] = thread;
	++(list->pos);
	return 1;
}

static int inclass(char* class, int len, char c) {
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

static regex_t submatch(regex_t re) {
	char bbin[9] = { 5, 4, 0, 6, 0, 2, 4, 7, 0 }; /* \0x5\0x4\0x0\0x6\0x0\0x2\0x4\0x7\0x0 */
	char fbin[10] = { 5, 4, 0, 6, 0, 2, 4, 7, 0, 11 }; /* \0x5\0x4\0x0\0x6\0x0\0x2\0x4\0x7\0x0\0xb */
	--re.len;
	regex_t new = (regex_t){ .bin = NULL, .len = 9 + re.len + 10 };
	new.bin = (char*)calloc(new.len + 1, sizeof(char));

	int i = 0;
	int j = 0;
	int k = 0;
	for(; i < 9; ++i)
		new.bin[i] = bbin[i];
	for(i = 9; j < re.len; ++j)
		new.bin[i++] = re.bin[j];
	for(i = 9 + re.len; k < 10; ++k)
		new.bin[i++] = fbin[k];

	return new;
}

int rematch(regex_t re, char* s) {
	int sub = 0;
	if(re.bin[0] != BLINE || re.bin[re.len - 2] != ELINE) {
		re = submatch(re);
		sub = 1;
	}

	Threadlist clist = (Threadlist){ .pos = 0 };
	clist.t = (int*)malloc(re.len * sizeof(int));
	Threadlist nlist = (Threadlist){ .pos = 0 };
	nlist.t = (int*)malloc(re.len * sizeof(int));
	Threadlist tmp;

	int pc = 0;
	int sp = 0;
	int s_len = strlen(s);

	clist.t[clist.pos++] = pc;
	for(; sp <= s_len; ++sp) {
		for(int i = 0; i < clist.pos; ++i) {
			pc = clist.t[i];
			char op = re.bin[pc];
			int a0 = pc + ARROW_0;
			int a1 = pc + ARROW_1;
			int cl = 0;

			switch(op) {
				case CHAR:
					pc += DATA;
					if(re.bin[pc] == s[sp])
						addthread(&nlist, pc + ARROW_0);
					break;

				case WILD:
					addthread(&nlist, a0);
					break;

				case JUMP:
					addthread(&clist, a0 + getnum(re.bin + a0));
					break;

				case BACK:
					addthread(&clist, a0 - getnum(re.bin + a0));
					break;

				case ALT:
					addthread(&clist, a0 + getnum(re.bin + a0));
					addthread(&clist, a1 + getnum(re.bin + a1));
					break;

				case CLASS:
					cl = getnum(re.bin + pc + DATA);
					if(inclass(re.bin + pc + CLASSBEGIN, cl, s[sp]))
						addthread(&nlist, pc + CLASSLEN + cl);
					break;

				case BLINE:
					if(sp == 0)
						addthread(&clist, a0);
					break;

				case ELINE:
					if(sp == (s_len - 1))
						addthread(&clist, a0);
					break;

				case MATCH:
					if(sp == s_len) {
						if(sub)
							free(re.bin);
						free(clist.t);
						free(nlist.t);
						return 1;
					}
					break;
			}
		}
		tmp = clist;
		clist = nlist;
		nlist = tmp;
		nlist.pos = 0;
	}
	free(clist.t);
	free(nlist.t);

	if(sub)
		free(re.bin);

	return 0;
}
