#include <stdlib.h>
#include <stdio.h>
#include "stack.h"

/*
 * A fixed size generic stack implementation
 */

/*
typedef struct {
	void* data;
	void* top;
	unsigned int max;
	unsigned int size;
} Stack;
*/

Stack nStack(int max, size_t TYPESIZE) {
	Stack s;
	s.data = calloc(max, TYPESIZE);
	s.top = s.data;
	s.nil = calloc(1, TYPESIZE);
	s.max = max;
	s.size = 0;
	return s;
}

void dStack(Stack* s) {
	free(s->data);
	s->data = s->top = NULL;
	free(s->nil);
	s->nil = NULL;
	s->max = s->size = 0;
}

/*
 * template for assign function:
 *
 * void assign_<type>(void* dest, void* src)
 * {
 * 	*((<type>*)dest) = *((<type>*)src);
 * }
 *
 */

// put value at top of Stack s in dest
void peek(Stack* s, void* dest, void(*assign)(void*, void*)) {
	assign(dest, s->top);
}

// add value at top of Stack
void pile(Stack* s, void* src, void(*assign)(void*, void*)) {
	int size = s->size;
	if(size == s->max - 1)
		return;
	if(size)
		assign(++(s->top), src);
	else
		assign(s->top, src);
	++(s->size);
}

// pop value off top of Stack and into dest
void pop(Stack* s, void* dest, void(*assign)(void*, void*)) {
	if(dest)
		assign(dest, s->top);
	assign(s->top, s->nil);
	if(s->top > s->data) {
		--(s->top);
		--(s->size);
	} else
		s->size = 0;
}

/*
void assign_char(void* dest, void* src) {
	*((char*)dest) = *((char*)src);
}

int main(void) {
	Stack s = nStack(4, sizeof(char));
	char c = 'a';
	pile(&s, (void*)(&c), assign_char);
	char k = 0;
	peek(&s, (void*)(&k), assign_char);
	printf("%c\n", k);
	dStack(&s);
	return 0;
}
*/
