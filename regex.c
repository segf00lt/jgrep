#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "const.h"
#include "helper.h"
#include "stack.h"

void assign_char(void* dest, void* src) {
	*((char*)dest) = *((char*)src);
}

void mvtop_char(Stack* s, int i) {
	s->top = (void*)((char*)(s->top) + i);
}

char* postfix_fmt(char* exp) {
	size_t l_exp = strlen(exp);
	if(!l_exp) {
		fprintf(stderr, "error: empty expression\n");
		return NULL;
	}

	Stack stack = nStack(l_exp + 1, CHARSIZE, mvtop_char, assign_char);

	char* buf = (char*)calloc((l_exp * 2) + 1, CHARSIZE);
	int i = 0;

	int n_atoms = 0;

	// store values of n_atoms prior to entering a group
	int* prev_atoms = (int*)calloc(l_exp + 1, CHARSIZE);
	int j = 0;

	inline void cleanup() {
		free(prev_atoms);
		free(buf);
		dStack(&stack);
	}

	for(char* c = exp; *c; ++c) {
		char tmp = 0;
		void* p = (void*)(&tmp);

		switch(*c) {
			case '(':
				if(n_atoms > 1) {
					buf[i++] = '&';
					--n_atoms;
				}

				prev_atoms[j++] = n_atoms;

				n_atoms = 0;

				pile(&stack, (void*)c);
				break;

			case '|':
				if(n_atoms == 0) {
					fprintf(stderr, "error: lone or incomplete alternation");
					cleanup();
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(peek(&stack, p); tmp != 0 && tmp != '('; peek(&stack, p))
					pop(&stack, (void*)(buf + (i++)));
				tmp = 0;

				pile(&stack, (void*)c);
				break;

			case ')':
				if(j == 0 || n_atoms == 0) {
					fprintf(stderr, "error: mismatched or empty parentheses\n");
					cleanup();
					return NULL;
				}

				for(--n_atoms; n_atoms > 0; --n_atoms)
					buf[i++] = '&';

				for(peek(&stack, p); tmp != 0 && tmp != '('; peek(&stack, p))
					pop(&stack, (void*)(buf + (i++)));
				tmp = 0;

				pop(&stack, NULL);

				// remember to decrement the position in prev_atoms
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
		// remeber to set tmp to 0
		tmp = 0;
	}

	if(j != 0) {
		fprintf(stderr, "error: mismatched parentheses\n");
		cleanup();
		return NULL;
	}

	for(--n_atoms; n_atoms > 0; --n_atoms)
		buf[i++] = '&';

	while(stack.size > 0)
		pop(&stack, (void*)(buf + (i++)));

	// remember to free post when done
	char* post = (char*)calloc((strlen(buf) + 1), CHARSIZE);
	strcpy(post, buf);

	cleanup();
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

void assign_Frag(void* dest, void* src) {
	*((Frag*)dest) = *((Frag*)src);
}

void mvtop_Frag(Stack* s, int i) {
	s->top = (void*)((Frag*)(s->top) + i);
}

void assign_Stateppp(void* dest, void* src) {
	*((State***)dest) = *((State***)src);
}

void patch(State*** out, State* s, int l_out) {
	for(int i = (l_out - 1); i >= 0; --i)
		*(out[i]) = s;
}

State* nfa_mk(char* post) {
	Stack stack = nStack(strlen(post), sizeof(Frag), mvtop_Frag, assign_Frag);

	/* make temporary variables */
	State* s = NULL;
	State*** o = NULL;
	Frag nilFrag = *((Frag*)(stack.nil));
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
			default:
				s = nState(REG, *c, NULL, NULL);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_0);

				frag = (Frag){ .start = s, .out = o, .l_out = 1 };
				pile(&stack, (void*)(&frag));

				cleartmp();
				break;

			case '&':
				pop(&stack, (void*)(&e2));
				pop(&stack, (void*)(&e1));

				patch(e1.out, e2.start, e1.l_out);
				free(e1.out);

				frag = (Frag){ .start = e1.start, .out = e2.out, .l_out = e2.l_out };
				pile(&stack, (void*)(&frag));

				cleartmp();
				break;

			case '|':
				pop(&stack, (void*)(&e2));
				pop(&stack, (void*)(&e1));

				s = nState(SPLIT, 0, e1.start, e2.start);
				o = (State***)calloc(e1.l_out + e2.l_out, sizeof(State**));

				gmemcpy((void*)o, (void*)(e1.out), e1.l_out, assign_Stateppp);
				gmemcpy((void*)(o + e1.l_out), (void*)(e2.out), e2.l_out, assign_Stateppp);

				free(e1.out);
				free(e2.out);

				frag = (Frag){ .start = s, .out = o, .l_out = (e1.l_out + e2.l_out) };
				pile(&stack, (void*)(&frag));

				cleartmp();
				break;

			case '*':
				pop(&stack, (void*)(&e1));

				s = nState(SPLIT, 0, e1.start, NULL);

				patch(e1.out, s, e1.l_out);
				free(e1.out);

				o = (State***)calloc(2, sizeof(State**));
				o[0] = &(s->out_0);
				o[1] = &(s->out_1);

				frag = (Frag){ .start = s, .out = o, .l_out = 2 };
				pile(&stack, (void*)(&frag));

				cleartmp();
				break;

			case '+':
				pop(&stack, (void*)(&e1));

				s = nState(SPLIT, 0, e1.start, NULL);

				patch(e1.out, s, e1.l_out);
				free(e1.out);

				o = (State***)malloc(sizeof(State**));
				*o = &(s->out_1);

				frag = (Frag){ .start = e1.start, .out = o, .l_out = 1 };
				pile(&stack, (void*)(&frag));

				cleartmp();
				break;

			case '?':
				pop(&stack, (void*)(&e1));

				s = nState(SPLIT, 0, e1.start, NULL);

				o = (State***)calloc(e1.l_out + 1, sizeof(State**));
				gmemcpy((void*)(o), (void*)(e1.out), e1.l_out, assign_Stateppp);
				*(o + e1.l_out) = &(s->out_1);

				free(e1.out);

				frag = (Frag){ .start = s, .out = o, .l_out = (e1.l_out + 1) };
				pile(&stack, (void*)(&frag));

				cleartmp();
				break;
		}
	}
	cleartmp();

	pop(&stack, (void*)(&e0));

	dStack(&stack);

	State* match = nState(MATCH, 0, NULL, NULL);

	patch(e0.out, match, e0.l_out);

	free(e0.out);

	return e0.start;
}

void dnfa(State* nfa) {
}


int main(int argc, char* argv[]) {
	char* post = postfix_fmt(argv[1]);
	printf("%s\n", post ? post : "");
	printf("%u\n", sizeof(Frag));
	/*
	State* nfa = nfa_mk(post);
	free(post);
	free((nfa->out_0)->out_0);
	free(nfa->out_0);
	free(nfa->out_1);
	free(nfa);
	*/

	return 0;
}
