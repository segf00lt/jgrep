/*
 * A fixed size generic stack implementation
 */

#ifndef STACK_H
#define STACK_H
typedef struct {
	void* data;
	void* top;
	void* nil;
	unsigned int max;
	unsigned int size;
} Stack;

Stack nStack(int max, size_t TYPESIZE);
void dStack(Stack* s);

// put value at top of Stack s in dest
void peek(Stack* s, void* dest, void(*assign)(void*, void*));

// add value at top of Stack
void pile(Stack* s, void* src, void(*assign)(void*, void*));

// pop value off top of Stack and into dest
void pop(Stack* s, void* dest, void(*assign)(void*, void*));
#endif
