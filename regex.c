#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "regex.h"

/* global variables */
char char_stack[MAX];
char buf[MAX];

/* store locals prior to entering group or class */
struct Prev {
	int atoms;
	char baseop;
};

struct Prev prev[MAX];

enum STATETYPE { REG, WILD, SPLIT, MATCH };
Frag nilFrag = (Frag){ .start = NULL, .out = NULL, .l_out = 0 };
State matchstate = (State){ .t = MATCH, .c = 0, .out_0 = NULL, .out_1 = NULL, .id = 0 };
Frag frag_stack[MAX];

List clist, nlist, t;

/* store all states allocated to facilitate deallocation */
State* all[MAX];
int all_pos = 0;

/* zero initialize globals */
void initglobal() {
	for(int i = 0; i < MAX; ++i) {
		char_stack[i] = buf[i] = '\000';
		prev[i] = (struct Prev){ .atoms = 0, .baseop = 0 };

		frag_stack[i] = nilFrag;

		clist.s[i] = nlist.s[i] = t.s[i] = NULL;

		all[i] = NULL;
	}
}


/* free states */
void nfa_del(void) {
	if(all_pos == 0)
		return;
	for(--all_pos; all_pos >= 0; --all_pos)
		free(all[all_pos]);
}


char* postfix_fmt(char* exp) {
	size_t l_exp = strlen(exp);
	if(!l_exp) {
		fprintf(stderr, "error: empty expression\n");
		return NULL;
	} else if(l_exp > MAX) {
		fprintf(stderr, "error: expression too large\n");
		return NULL;
	}

	/* position in char_stack */
	int stackpos = 0;
	/* size of char_stack */
	int stacksize = 0;

	/* position in buf */
	int i = 0;

	int n_atoms = 0;

	/* position in prev */
	int j = 0;

	char baseop = '&';

	char nc = 0;

	for(char* c = exp; *c; ++c) {
		switch(*c) {
			case '(':
				if(baseop == '|')
					break;

				if(n_atoms > 1) {
					buf[i++] = baseop;
					--n_atoms;
				}

				prev[j].baseop = baseop;
				prev[j++].atoms = n_atoms;

				baseop = '&';
				n_atoms = 0;

				if(stacksize > 0)
					++stackpos;
				else
					stackpos = 0;

				char_stack[stackpos] = *c;
				++stacksize;
				break;

			case '|':
				if(n_atoms == 0) {
					fprintf(stderr, "error: lone or incomplete alternation");
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(; stacksize > 0 && stackpos >= 0 && char_stack[stackpos] != '('; --stacksize)
				{
					buf[i++] = char_stack[stackpos];
					char_stack[stackpos--] = 0;
				}

				if(stacksize > 0)
					++stackpos;
				else
					stackpos = 0;

				char_stack[stackpos] = *c;
				++stacksize;
				break;

			case ')':
				if(baseop == '|')
					break;

				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "error: mismatched or empty parentheses\n");
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(; stacksize > 0 && stackpos >= 0 && char_stack[stackpos] != '('; --stacksize)
				{
					buf[i++] = char_stack[stackpos];
					char_stack[stackpos--] = 0;
				}

				if(stacksize == 0) {
					stackpos = 0;
					char_stack[stackpos] = 0;
				} else {
					char_stack[stackpos--] = 0;
					--stacksize;
				}

				baseop = prev[--j].baseop;
				n_atoms = prev[j].atoms + 1;
				break;

			case '[':
				if(n_atoms > 1) {
					buf[i++] = baseop;
					--n_atoms;
				}

				prev[j].baseop = baseop;
				prev[j++].atoms = n_atoms;

				baseop = '|';
				n_atoms = 0;

				if(stacksize > 0)
					++stackpos;
				else
					stackpos = 0;

				char_stack[stackpos] = *c;
				++stacksize;
				break;

			case '-':
				break;

			case ']':
				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "error: mismatched or empty square brackets\n");
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '|';

				for(; stacksize > 0 && stackpos >= 0 && char_stack[stackpos] != '['; --stacksize)
				{
					buf[i++] = char_stack[stackpos];
					char_stack[stackpos--] = 0;
				}

				if(stacksize == 0) {
					stackpos = 0;
					char_stack[stackpos] = 0;
				} else {
					char_stack[stackpos--] = 0;
					--stacksize;
				}

				baseop = prev[--j].baseop;
				n_atoms = prev[j].atoms + 1;
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

			case '\\':
				if(n_atoms > 1) {
					--n_atoms;
					buf[i++] = baseop;
				}

				nc = *(c + 1);
				if(nc == 0) {
					fprintf(stderr, "error: lone escape operator\n");
					return NULL;
				} else if(nc == '.' || nc == '&' || nc == '\\')
					buf[i++] = *c;
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

	if(j != 0) {
		fprintf(stderr, "error: mismatched parentheses or square brackets\n");
		return NULL;
	}

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	for(; stacksize > 0; --stacksize) {
		buf[i++] = char_stack[stackpos];
		char_stack[stackpos--] = 0;
	}

	return buf;
}


State* nState(int t, char c, State* out_0, State* out_1) {
	State* s = (State*)malloc(sizeof(State));
	s->t = t;
	s->c = c;
	s->out_0 = out_0;
	s->out_1 = out_1;
	s->id = 0;
	return s;
}

void patch(State*** out, State* s, int l_out) {
	for(int i = (l_out - 1); i >= 0; --i)
		*(out[i]) = s;
}

State* nfa_mk(char* post) {
	if(!post)
		return NULL;

	/* position in frag_stack */
	int stackpos = 0;
	/* size of frag_stack */
	int stacksize = 0;

	/* make temporary variables */
	State* s = NULL;
	State*** o = NULL;
	Frag frag, e0, e1, e2;
	frag = e0 = e1 = e2 = nilFrag;

	/* clear temporary variables */
	inline void cleartmp() {
		s = NULL;
		o = NULL;
		frag = e0 = e1 = e2 = nilFrag;
	}

	for(char*c = post; *c; ++c) {
		switch(*c) {
			case '\\':
				++c;

			default:
				s = nState(REG, *c, NULL, NULL);
				all[all_pos++] = s;

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_0);

				frag = (Frag){ .start = s, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '.':
				s = nState(WILD, 0, NULL, NULL);
				all[all_pos++] = s;

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_0);

				frag = (Frag){ .start = s, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '&':
				e2 = frag_stack[stackpos];
				frag_stack[stackpos--] = nilFrag;
				e1 = frag_stack[stackpos];
				frag_stack[stackpos] = nilFrag;
				stacksize -= 2;

				if(stacksize > 0)
					--stackpos;

				patch(e1.out, e2.start, e1.l_out);
				free(e1.out);

				frag = (Frag){ .start = e1.start, .out = e2.out, .l_out = e2.l_out };

				if(stacksize > 0)
					++stackpos;

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '|':
				e2 = frag_stack[stackpos];
				frag_stack[stackpos--] = nilFrag;
				e1 = frag_stack[stackpos];
				frag_stack[stackpos] = nilFrag;
				stacksize -= 2;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, e2.start);
				all[all_pos++] = s;
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

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '*':
				e1 = frag_stack[stackpos];
				frag_stack[stackpos] = nilFrag;
				--stacksize;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, NULL);
				all[all_pos++] = s;

				patch(e1.out, s, e1.l_out);
				free(e1.out);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_1);

				frag = (Frag){ .start = s, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '+':
				e1 = frag_stack[stackpos];
				frag_stack[stackpos] = nilFrag;
				--stacksize;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, NULL);
				all[all_pos++] = s;

				patch(e1.out, s, e1.l_out);
				free(e1.out);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_1);

				frag = (Frag){ .start = e1.start, .out = o, .l_out = 1 };

				if(stacksize > 0)
					++stackpos;

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;

			case '?':
				e1 = frag_stack[stackpos];
				frag_stack[stackpos] = nilFrag;
				--stacksize;

				if(stacksize > 0)
					--stackpos;

				s = nState(SPLIT, 0, e1.start, NULL);
				all[all_pos++] = s;

				o = (State***)calloc(e1.l_out + 1, sizeof(State**));

				for(size_t l = 0; l < e1.l_out; ++l)
					o[l] = e1.out[l];

				*(o + e1.l_out) = &(s->out_1);

				free(e1.out);

				frag = (Frag){ .start = s, .out = o, .l_out = (e1.l_out + 1) };

				if(stacksize > 0)
					++stackpos;

				frag_stack[stackpos] = frag;
				++stacksize;

				cleartmp();
				break;
		}
	}
	cleartmp();

	e0 = frag_stack[stackpos];
	frag_stack[stackpos] = nilFrag;
	--stacksize;

	patch(e0.out, &matchstate, e0.l_out);

	free(e0.out);

	return e0.start;
}


int listid = 0;

void addstate(State* s, List* l) {
	if(s == NULL || s->id == listid)
		return;
	s->id = listid;

	if(s->t == SPLIT) {
		addstate(s->out_0, l);
		addstate(s->out_1, l);
		return;
	}

	l->s[l->n++] = s;
}

void step(List* clist, List* nlist, char c) {
	State* s;
	nlist->n = 0;
	++listid;

	for(int i = 0; i < clist->n; ++i) {
		s = clist->s[i];
		if(s->c == c || s->t == WILD)
			addstate(s->out_0, nlist);
	}
}

int match(State* start, char* str) {
	clist.n = nlist.n = 0;

	++listid;
	addstate(start, &clist);

	for(; *str; ++str) {
		step(&clist, &nlist, *str);
		t = clist;
		clist = nlist;
		nlist = t;
	}

	int ismatch = 0;
	for(int i = 0; i < clist.n; ++i) {
		if(clist.s[i] == &matchstate) {
			ismatch = 1;
			break;
		}
	}

	return ismatch;
}

int main(int argc, char* argv[]) {
	initglobal();
	char* post = postfix_fmt(argv[1]);
	if(post)
		printf("%s\n", post);
	State* nfa = nfa_mk(post);
	int res = match(nfa, argv[2]);
	printf("%s\n", res ? "match" : "no match");
	nfa_del();
	return 0;
}
