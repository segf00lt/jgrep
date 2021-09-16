#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* max size for stacks and buffers */
#define MAX 512

char* postfix_fmt(char* exp) {
	size_t l_exp = strlen(exp);
	if(!l_exp) {
		fprintf(stderr, "error: empty expression\n");
		return NULL;
	} else if(l_exp > MAX) {
		fprintf(stderr, "error: expression too large\n");
		return NULL;
	}

	char stack[MAX];
	int stackpos = 0;
	int stacksize = 0;

	char buf[MAX];
	int i = 0;

	int n_atoms = 0;

	/* store values of n_atoms prior to entering a group */
	int prev_atoms[MAX];
	int j = 0;

	/* zero initialize memory */
	for(int n = 0; n < MAX; ++n) {
		stack[n] = buf[n] = 0;
		prev_atoms[n] = 0;
	}

	for(char* c = exp; *c; ++c) {
		switch(*c) {
			case '(':
				if(n_atoms > 1) {
					buf[i++] = '&';
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
				if(n_atoms == 0) {
					fprintf(stderr, "error: lone or incomplete alternation");
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(; stacksize > 0 && stackpos >= 0 && stack[stackpos] != '('; --stacksize) {
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
				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "error: mismatched or empty parentheses\n");
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(; stacksize > 0 && stackpos >= 0 && stack[stackpos] != '('; --stacksize) {
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

			case '*':
				if(n_atoms == 0) {
					fprintf(stderr, "error: lone repetition operator\n");
					return NULL;
				}
				buf[i++] = *c;
				break;

			case '+':
				if(n_atoms == 0) {
					fprintf(stderr, "error: lone repetition operator\n");
					return NULL;
				}
				buf[i++] = *c;
				break;

			case '?':
				if(n_atoms == 0) {
					fprintf(stderr, "error: lone repetition operator\n");
					return NULL;
				}
				buf[i++] = *c;
				break;

			default:
				if(n_atoms > 1) {
					--n_atoms;
					buf[i++] = '&';
				}

				buf[i++] = *c;
				++n_atoms;
				break;

		}
	}

	if(j != 0) {
		fprintf(stderr, "error: mismatched parentheses\n");
		return NULL;
	}

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	for(; stacksize > 0; --stacksize) {
		buf[i++] = stack[stackpos];
		stack[stackpos--] = 0;
	}

	/* remember to free post when done */
	char* post = (char*)calloc((strlen(buf) + 1), sizeof(char));
	strcpy(post, buf);

	return post;
}


enum STATETYPE { REG, SPLIT, MATCH };

typedef struct State {
	int t;
	char c;
	struct State* out_0;
	struct State* out_1;
} State;

State* nState(int t, char c, State* out_0, State* out_1) {
	State* s = (State*)malloc(sizeof(State));
	s->t = t;
	s->c = c;
	s->out_0 = out_0;
	s->out_1 = out_1;
	return s;
}

typedef struct {
	State* start;
	State*** out;
	int l_out;
} Frag;

State match = (State){ .t = MATCH, .c = 0, .out_0 = NULL, .out_1 = NULL };

void patch(State*** out, State* s, int l_out) {
	for(int i = (l_out - 1); i >= 0; --i)
		*(out[i]) = s;
}

State* nfa_mk(char* post) {
	Frag stack[MAX];
	int stackpos = 0;
	int stacksize = 0;

	/* make temporary variables */
	State* s = NULL;
	State*** o = NULL;
	Frag nilFrag = (Frag){ .start = NULL, .out = NULL, .l_out = 0 };
	Frag frag, e0, e1, e2;
	frag = e0 = e1 = e2 = nilFrag;

	/* clear temporary variables */
	inline void cleartmp() {
		s = NULL;
		o = NULL;
		frag = e0 = e1 = e2 = nilFrag;
	}

	/* zero initialize stack */
	for(int n = 0; n < MAX; ++n)
		stack[n] = nilFrag;

	for(char*c = post; *c; ++c) {
		switch(*c) {
			default:
				s = nState(REG, *c, NULL, NULL);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_0);

				frag = (Frag){ .start = s, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '&':
				e2 = stack[stackpos];
				stack[stackpos--] = nilFrag;
				e1 = stack[stackpos];
				stack[stackpos] = nilFrag;
				stacksize -= 2;

				if(stacksize > 0)
					--stackpos;

				patch(e1.out, e2.start, e1.l_out);
				free(e1.out);

				frag = (Frag){ .start = e1.start, .out = e2.out, .l_out = e2.l_out };

				if(stacksize > 0)
					++stackpos;

				stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '|':
				e2 = stack[stackpos];
				stack[stackpos--] = nilFrag;
				e1 = stack[stackpos];
				stack[stackpos] = nilFrag;
				stacksize -= 2;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, e2.start);
				o = (State***)calloc(e1.l_out + e2.l_out, sizeof(State**));

				for(size_t l = 0; l < e1.l_out; ++l)
					o[l] = e1.out[l];

				for(size_t l = 0; l < e2.l_out; ++l)
					*((o + e1.l_out) + l) = e2.out[l];

				free(e1.out);
				free(e2.out);

				frag = (Frag){ .start = s, .out = o, .l_out = (e1.l_out + e2.l_out) };

				if(stacksize > 0)
					++stackpos;

				stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '*':
				e1 = stack[stackpos];
				stack[stackpos] = nilFrag;
				--stacksize;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, NULL);

				patch(e1.out, s, e1.l_out);
				free(e1.out);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_1);

				frag = (Frag){ .start = s, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '+':
				e1 = stack[stackpos];
				stack[stackpos] = nilFrag;
				--stacksize;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, NULL);

				patch(e1.out, s, e1.l_out);
				free(e1.out);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_1);

				frag = (Frag){ .start = e1.start, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '?':
				e1 = stack[stackpos];
				stack[stackpos] = nilFrag;
				--stacksize;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, NULL);

				o = (State***)calloc(e1.l_out + 1, sizeof(State**));

				for(size_t l = 0; l < e1.l_out; ++l)
					o[l] = e1.out[l];

				*(o + e1.l_out) = &(s->out_1);

				free(e1.out);

				frag = (Frag){ .start = s, .out = o, .l_out = (e1.l_out + 1) };

				if(stacksize > 0)
					++stackpos;

				stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;
		}
	}
	cleartmp();

	e0 = stack[stackpos];
	stack[stackpos] = nilFrag;
	--stacksize;

	patch(e0.out, &match, e0.l_out);

	free(e0.out);

	return e0.start;
}

int main(int argc, char* argv[]) {
	char* post = postfix_fmt(argv[1]);
	printf("%s\n", post ? post : "");
	State* nfa = nfa_mk(post);
	free(nfa->out_1->out_0);
	free(nfa->out_1->out_1);
	free(nfa->out_1);
	free(nfa->out_0);
	free(nfa);
	free(post);

	return 0;
}
