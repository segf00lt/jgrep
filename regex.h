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

typedef State* regex_t;

void initglobal();
char* crange(char f, char t);
char* postfix_fmt(char* exp);
void patch(State*** out, State* s, int l_out);
State* nfa_mk(char* post);
void nfa_del(void);
regex_t recomp(char* exp);
void addstate(State* s, List* l);
int rematch(regex_t start, char* str);
int rematchsub(State* start, char* str);

#endif
