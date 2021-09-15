#include <stdlib.h>
#include <stdio.h>
#include "stack.h"

/*
 * A fixed size generic stack implementation
 */

/*
 * template for assign function:
 *
 * void assign_<type>(void* dest, void* src)
 * {
 * 	*((<type>*)dest) = *((<type>*)src);
 * }
 *
 * template for mvtop function:
 *
 * void mvtop_<type>(Stack* s, int i)
 * {
 * 	s->top = (void*)((<type>*)(s->top) + i);
 * }
 */

Stack nStack(int max, size_t TYPE, void(*mvtop)(Stack*, int), void(*assign)(void*, void*)) {
	Stack s;
	s.data = calloc(max + 1, TYPE);
	s.top = s.data;
	s.nil = calloc(1, TYPE);
	s.mvtop = mvtop;
	s.assign = assign;
	s.max = max;
	s.size = 0;
	return s;
}

void dStack(Stack* s) {
	free(s->data);
	s->data = s->top = NULL;
	free(s->nil);
	s->nil = NULL;
	s->mvtop = NULL;
	s->assign = NULL;
	s->max = s->size = 0;
}

// put value at top of Stack s in dest
void peek(Stack* s, void* dest) {
	s->assign(dest, s->top);
}

// add value at top of Stack
void pile(Stack* s, void* src) {
	unsigned int size = s->size;
	if(size == s->max)
		return;
	if(size != 0)
		s->mvtop(s, 1);
	s->assign(s->top, src);
	++(s->size);
}

// pop value off top of Stack and into dest
void pop(Stack* s, void* dest) {
	if(dest)
		s->assign(dest, s->top);
	s->assign(s->top, s->nil);
	if(s->top > s->data) {
		s->mvtop(s, -1);
		--(s->size);
	} else
		s->size = 0;
}
