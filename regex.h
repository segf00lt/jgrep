/*
 * A regex implementation
 */

#ifndef REGEX_H
#define REGEX_H

/* max size for stacks and buffers */
#define MAX 512

typedef struct State {
	int t;
	char c;
	struct State* out_0;
	struct State* out_1;
	int id;
} State;

typedef struct {
	State* start;
	State*** out;
	int l_out;
} Frag;

typedef struct {
	State* s[MAX];
	int n;
} List;

void initglobal();
void nfa_del(void);
char* postfix_fmt(char* exp);
void patch(State*** out, State* s, int l_out);
State* nfa_mk(char* post);
void addstate(State* s, List* l);
void step(List* clist, List* nlist, char c);
int match(State* start, char* str);

#endif
