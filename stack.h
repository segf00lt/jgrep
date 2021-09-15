/*
 * A fixed size generic stack implementation
 */

#ifndef STACK_H
#define STACK_H

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

typedef struct Stack{
	void* data;
	void* top;
	void* nil;

	void(*mvtop)(struct Stack*, int);
	void(*assign)(void*, void*);

	unsigned int max;
	unsigned int size;
} Stack;

Stack nStack(int max, size_t TYPE, void(*mvtop)(Stack*, int), void(*assign)(void*, void*));
void dStack(Stack* s);

// put value at top of Stack s in dest
void peek(Stack* s, void* dest);

// add value at top of Stack
void pile(Stack* s, void* src);

// pop value off top of Stack and into dest
void pop(Stack* s, void* dest);
#endif
